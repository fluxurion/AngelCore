/*
 * ASSpawnDB.cpp — AngelDB-backed persistence for AngelScript spawns
 *
 * All SQL goes through AngelDB, completely separate from TC's world DB.
 */

#include "ASSpawnDB.h"
#include "ASAngelDB.h"
#include "Log.h"
#include <cstdio>

namespace AngelScript
{
    // Static counter initialization — overwritten by SyncCountersFromDB()
    std::atomic<ObjectGuid::LowType> ASSpawnDB::s_nextCreatureSpawnId   = ASSpawnDB::AS_SPAWN_GUID_FLAG | 1;
    std::atomic<ObjectGuid::LowType> ASSpawnDB::s_nextGameObjectSpawnId = ASSpawnDB::AS_SPAWN_GUID_FLAG | 1;

    // ========================================================================
    // GUID generation
    // ========================================================================
    ObjectGuid::LowType ASSpawnDB::GenerateCreatureSpawnId()
    {
        ObjectGuid::LowType id = s_nextCreatureSpawnId.fetch_add(1);
        // Safety: ensure we never exceed the 40-bit counter space
        if (id >= UI64LIT(0x000000FFFFFFFFFF))
        {
            TC_LOG_ERROR("server.angelscript",
                "[ASSpawnDB] Creature spawn ID overflow! Cannot generate more IDs.");
            return 0;
        }
        return id;
    }

    ObjectGuid::LowType ASSpawnDB::GenerateGameObjectSpawnId()
    {
        ObjectGuid::LowType id = s_nextGameObjectSpawnId.fetch_add(1);
        if (id >= UI64LIT(0x000000FFFFFFFFFF))
        {
            TC_LOG_ERROR("server.angelscript",
                "[ASSpawnDB] GameObject spawn ID overflow! Cannot generate more IDs.");
            return 0;
        }
        return id;
    }

    // ========================================================================
    // Sync counters from AngelDB on startup
    // ========================================================================
    void ASSpawnDB::SyncCountersFromDB()
    {
        if (!ASAngelDB::Instance().IsConnected())
        {
            TC_LOG_WARN("server.angelscript",
                "[ASSpawnDB] AngelDB not connected, using default GUID counters");
            return;
        }

        // Read next_creature_spawn_id
        {
            auto* result = static_cast<ASAngelDBResult*>(
                ASAngelDB::Instance().QueryLocked(
                    "SELECT `value` FROM `as_spawn_config` WHERE `key`='next_creature_spawn_id'"));
            if (result && result->rowCount > 0)
            {
                result->NextRow();
                ObjectGuid::LowType dbVal = result->GetUInt64(0);
                if (dbVal >= s_nextCreatureSpawnId.load())
                {
                    s_nextCreatureSpawnId.store(dbVal);
                    TC_LOG_INFO("server.angelscript",
                        "[ASSpawnDB] Synced creature spawn ID from DB: {}", dbVal);
                }
            }
            delete result;
        }

        // Read next_gameobject_spawn_id
        {
            auto* result = static_cast<ASAngelDBResult*>(
                ASAngelDB::Instance().QueryLocked(
                    "SELECT `value` FROM `as_spawn_config` WHERE `key`='next_gameobject_spawn_id'"));
            if (result && result->rowCount > 0)
            {
                result->NextRow();
                ObjectGuid::LowType dbVal = result->GetUInt64(0);
                if (dbVal >= s_nextGameObjectSpawnId.load())
                {
                    s_nextGameObjectSpawnId.store(dbVal);
                    TC_LOG_INFO("server.angelscript",
                        "[ASSpawnDB] Synced gameobject spawn ID from DB: {}", dbVal);
                }
            }
            delete result;
        }
    }

    // ========================================================================
    // Creature CRUD
    // ========================================================================
    bool ASSpawnDB::InsertCreatureSpawn(const ASPersistCreatureSpawn& spawn)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return false;

        char sql[2048];
        std::snprintf(sql, sizeof(sql),
            "INSERT INTO `as_creature_spawns` "
            "(`guid`,`entry`,`map`,`position_x`,`position_y`,`position_z`,`orientation`,"
            "`phaseId`,`respawnDelaySecs`,`spawntimesecs`,`wander_distance`,`movementType`,"
            "`npcflag`,`unit_flags`,`unit_flags2`,`faction`,`level`,`equipment_id`,"
            "`gossipMenuId`,`reactState`,`display_id`,`script_name`,`string_id`,`is_active`) "
            "VALUES (%llu,%u,%u,%f,%f,%f,%f,"
            "%u,%u,%u,%f,%u,"
            "%s,%s,%s,%u,%u,%u,"
            "%u,%u,%u,'%s',%s,1)",
            (unsigned long long)spawn.spawnId,
            spawn.entry,
            spawn.mapId,
            spawn.x, spawn.y, spawn.z, spawn.o,
            spawn.phaseId,
            spawn.respawnDelaySecs,
            spawn.spawntimesecs,
            spawn.wander_distance,
            spawn.movementType,
            spawn.npcflag   ? std::to_string(spawn.npcflag).c_str() : "NULL",
            spawn.unit_flags  ? std::to_string(spawn.unit_flags).c_str() : "NULL",
            spawn.unit_flags2 ? std::to_string(spawn.unit_flags2).c_str() : "NULL",
            spawn.faction,
            spawn.level,
            spawn.equipment_id,
            spawn.gossipMenuId,
            spawn.reactState,
            spawn.display_id,
            ASAngelDB::Instance().EscapeString(spawn.scriptName).c_str(),
            spawn.stringId.empty() ? "NULL" :
                ("'" + ASAngelDB::Instance().EscapeString(spawn.stringId) + "'").c_str());

        bool ok = ASAngelDB::Instance().Execute(sql);
        if (!ok)
            TC_LOG_ERROR("server.angelscript",
                "[ASSpawnDB] Failed to insert creature spawn guid={}", spawn.spawnId);
        return ok;
    }

    bool ASSpawnDB::UpdateCreatureSpawn(const ASPersistCreatureSpawn& spawn)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return false;

        char sql[2048];
        std::snprintf(sql, sizeof(sql),
            "UPDATE `as_creature_spawns` SET "
            "`entry`=%u,`map`=%u,`position_x`=%f,`position_y`=%f,`position_z`=%f,`orientation`=%f,"
            "`phaseId`=%u,`respawnDelaySecs`=%u,`is_active`=%u "
            "WHERE `guid`=%llu",
            spawn.entry, spawn.mapId,
            spawn.x, spawn.y, spawn.z, spawn.o,
            spawn.phaseId, spawn.respawnDelaySecs,
            spawn.isActive ? 1u : 0u,
            (unsigned long long)spawn.spawnId);

        return ASAngelDB::Instance().Execute(sql);
    }

    bool ASSpawnDB::DeleteCreatureSpawn(ObjectGuid::LowType spawnId)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return false;

        char sql[128];
        std::snprintf(sql, sizeof(sql),
            "DELETE FROM `as_creature_spawns` WHERE `guid`=%llu",
            (unsigned long long)spawnId);

        return ASAngelDB::Instance().Execute(sql);
    }

    void ASSpawnDB::MarkCreatureInactive(ObjectGuid::LowType spawnId)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return;

        char sql[128];
        std::snprintf(sql, sizeof(sql),
            "UPDATE `as_creature_spawns` SET `is_active`=0 WHERE `guid`=%llu",
            (unsigned long long)spawnId);

        ASAngelDB::Instance().Execute(sql);
    }

    // ========================================================================
    // GameObject CRUD
    // ========================================================================
    bool ASSpawnDB::InsertGameObjectSpawn(const ASPersistGameObjectSpawn& spawn)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return false;

        char sql[2048];
        std::snprintf(sql, sizeof(sql),
            "INSERT INTO `as_gameobject_spawns` "
            "(`guid`,`entry`,`map`,`position_x`,`position_y`,`position_z`,`orientation`,"
            "`rotation0`,`rotation1`,`rotation2`,`rotation3`,"
            "`phaseId`,`respawnDelaySecs`,`spawntimesecs`,"
            "`goState`,`animprogress`,`artKit`,`script_name`,`string_id`,`is_active`) "
            "VALUES (%llu,%u,%u,%f,%f,%f,%f,"
            "%f,%f,%f,%f,"
            "%u,%u,%u,"
            "%u,%u,%u,'%s',%s,1)",
            (unsigned long long)spawn.spawnId,
            spawn.entry,
            spawn.mapId,
            spawn.x, spawn.y, spawn.z, spawn.o,
            spawn.rot0, spawn.rot1, spawn.rot2, spawn.rot3,
            spawn.phaseId,
            spawn.respawnDelaySecs,
            spawn.spawntimesecs,
            spawn.goState,
            spawn.animprogress,
            spawn.artKit,
            ASAngelDB::Instance().EscapeString(spawn.scriptName).c_str(),
            spawn.stringId.empty() ? "NULL" :
                ("'" + ASAngelDB::Instance().EscapeString(spawn.stringId) + "'").c_str());

        bool ok = ASAngelDB::Instance().Execute(sql);
        if (!ok)
            TC_LOG_ERROR("server.angelscript",
                "[ASSpawnDB] Failed to insert gameobject spawn guid={}", spawn.spawnId);
        return ok;
    }

    bool ASSpawnDB::UpdateGameObjectSpawn(const ASPersistGameObjectSpawn& spawn)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return false;

        char sql[1024];
        std::snprintf(sql, sizeof(sql),
            "UPDATE `as_gameobject_spawns` SET "
            "`entry`=%u,`map`=%u,`position_x`=%f,`position_y`=%f,`position_z`=%f,`orientation`=%f,"
            "`phaseId`=%u,`respawnDelaySecs`=%u,`is_active`=%u "
            "WHERE `guid`=%llu",
            spawn.entry, spawn.mapId,
            spawn.x, spawn.y, spawn.z, spawn.o,
            spawn.phaseId, spawn.respawnDelaySecs,
            spawn.isActive ? 1u : 0u,
            (unsigned long long)spawn.spawnId);

        return ASAngelDB::Instance().Execute(sql);
    }

    bool ASSpawnDB::DeleteGameObjectSpawn(ObjectGuid::LowType spawnId)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return false;

        char sql[128];
        std::snprintf(sql, sizeof(sql),
            "DELETE FROM `as_gameobject_spawns` WHERE `guid`=%llu",
            (unsigned long long)spawnId);

        return ASAngelDB::Instance().Execute(sql);
    }

    void ASSpawnDB::MarkGameObjectInactive(ObjectGuid::LowType spawnId)
    {
        if (!ASAngelDB::Instance().IsConnected())
            return;

        char sql[128];
        std::snprintf(sql, sizeof(sql),
            "UPDATE `as_gameobject_spawns` SET `is_active`=0 WHERE `guid`=%llu",
            (unsigned long long)spawnId);

        ASAngelDB::Instance().Execute(sql);
    }

    // ========================================================================
    // Bulk load from AngelDB
    // ========================================================================
    std::vector<ASPersistCreatureSpawn> ASSpawnDB::LoadAllCreatureSpawns()
    {
        std::vector<ASPersistCreatureSpawn> spawns;

        if (!ASAngelDB::Instance().IsConnected())
        {
            TC_LOG_WARN("server.angelscript",
                "[ASSpawnDB] AngelDB not connected, skipping creature spawn load");
            return spawns;
        }

        auto* result = static_cast<ASAngelDBResult*>(
            ASAngelDB::Instance().QueryLocked(
                "SELECT `guid`,`entry`,`map`,`position_x`,`position_y`,`position_z`,`orientation`,"
                "`phaseId`,`respawnDelaySecs`,`spawntimesecs`,`wander_distance`,`movementType`,"
                "`npcflag`,`unit_flags`,`unit_flags2`,`faction`,`level`,`equipment_id`,"
                "`gossipMenuId`,`reactState`,`display_id`,`script_name`,`string_id`,`is_active` "
                "FROM `as_creature_spawns` WHERE `is_active`=1"));

        if (!result)
            return spawns;

        spawns.reserve(static_cast<size_t>(result->rowCount));

        for (uint64 i = 0; i < result->rowCount; ++i)
        {
            if (!result->NextRow())
                break;

            ASPersistCreatureSpawn s;
            s.spawnId         = result->GetUInt64(0);
            s.entry           = result->GetUInt32(1);
            s.mapId           = result->GetUInt32(2);
            s.x               = result->GetFloat(3);
            s.y               = result->GetFloat(4);
            s.z               = result->GetFloat(5);
            s.o               = result->GetFloat(6);
            s.phaseId         = result->GetUInt32(7);
            s.respawnDelaySecs= result->GetUInt32(8);
            s.spawntimesecs   = result->GetUInt32(9);
            s.wander_distance = result->GetFloat(10);
            s.movementType    = static_cast<uint8>(result->GetUInt32(11));
            s.npcflag         = result->IsNull(12) ? 0 : result->GetUInt64(12);
            s.unit_flags      = result->IsNull(13) ? 0 : result->GetUInt32(13);
            s.unit_flags2     = result->IsNull(14) ? 0 : result->GetUInt32(14);
            s.faction         = result->GetUInt32(15);
            s.level           = static_cast<uint8>(result->GetUInt32(16));
            s.equipment_id    = static_cast<uint8>(result->GetUInt32(17));
            s.gossipMenuId    = result->GetUInt32(18);
            s.reactState      = static_cast<uint8>(result->GetUInt32(19));
            s.display_id      = result->GetUInt32(20);
            s.scriptName      = result->GetString(21);
            s.stringId        = result->IsNull(22) ? "" : result->GetString(22);
            s.isActive        = result->GetUInt32(23) != 0;
            s.isPersisted     = true;
            // guid will be set after spawning
            spawns.push_back(std::move(s));
        }

        delete result;

        TC_LOG_INFO("server.angelscript",
            "[ASSpawnDB] Loaded {} creature spawns from AngelDB", spawns.size());
        return spawns;
    }

    std::vector<ASPersistGameObjectSpawn> ASSpawnDB::LoadAllGameObjectSpawns()
    {
        std::vector<ASPersistGameObjectSpawn> spawns;

        if (!ASAngelDB::Instance().IsConnected())
        {
            TC_LOG_WARN("server.angelscript",
                "[ASSpawnDB] AngelDB not connected, skipping gameobject spawn load");
            return spawns;
        }

        auto* result = static_cast<ASAngelDBResult*>(
            ASAngelDB::Instance().QueryLocked(
                "SELECT `guid`,`entry`,`map`,`position_x`,`position_y`,`position_z`,`orientation`,"
                "`rotation0`,`rotation1`,`rotation2`,`rotation3`,"
                "`phaseId`,`respawnDelaySecs`,`spawntimesecs`,"
                "`goState`,`animprogress`,`artKit`,`script_name`,`string_id`,`is_active` "
                "FROM `as_gameobject_spawns` WHERE `is_active`=1"));

        if (!result)
            return spawns;

        spawns.reserve(static_cast<size_t>(result->rowCount));

        for (uint64 i = 0; i < result->rowCount; ++i)
        {
            if (!result->NextRow())
                break;

            ASPersistGameObjectSpawn s;
            s.spawnId         = result->GetUInt64(0);
            s.entry           = result->GetUInt32(1);
            s.mapId           = result->GetUInt32(2);
            s.x               = result->GetFloat(3);
            s.y               = result->GetFloat(4);
            s.z               = result->GetFloat(5);
            s.o               = result->GetFloat(6);
            s.rot0            = result->GetFloat(7);
            s.rot1            = result->GetFloat(8);
            s.rot2            = result->GetFloat(9);
            s.rot3            = result->GetFloat(10);
            s.phaseId         = result->GetUInt32(11);
            s.respawnDelaySecs= result->GetUInt32(12);
            s.spawntimesecs   = result->GetUInt32(13);
            s.goState         = static_cast<uint8>(result->GetUInt32(14));
            s.animprogress    = static_cast<uint8>(result->GetUInt32(15));
            s.artKit          = result->GetUInt32(16);
            s.scriptName      = result->GetString(17);
            s.stringId        = result->IsNull(18) ? "" : result->GetString(18);
            s.isActive        = result->GetUInt32(19) != 0;
            s.isPersisted     = true;
            spawns.push_back(std::move(s));
        }

        delete result;

        TC_LOG_INFO("server.angelscript",
            "[ASSpawnDB] Loaded {} gameobject spawns from AngelDB", spawns.size());
        return spawns;
    }

} // namespace AngelScript
