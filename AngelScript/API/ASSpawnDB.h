/*
 * ASSpawnDB.h — AngelDB-backed persistence for AngelScript spawns
 *
 * Stores AS-spawned creatures & gameobjects in angelcore_db.
 * Uses an isolated GUID range (bit 39 set) to avoid collision with TC spawns.
 *
 * GUID separation:
 *   TC spawns     : 0x0000000001 .. 0x0000007FFFFFFFFF  (bit 39 clear)
 *   AngelCore     : 0x0000008000000001 .. 0x000000FFFFFFFFFF  (bit 39 set)
 *
 * The 40-bit counter field in ObjectGuid encoding preserves this distinction.
 */

#ifndef ASSPAWNDB_H
#define ASSPAWNDB_H

#ifdef ANGELSCRIPT_INTEGRATION

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <atomic>

class Creature;
class GameObject;
class Map;

namespace AngelScript
{
    // ------------------------------------------------------------------------
    // In-memory spawn entry — mirrors as_creature_spawns row + runtime state
    // ------------------------------------------------------------------------
    struct ASPersistCreatureSpawn
    {
        ObjectGuid guid;             // world-object GUID
        ObjectGuid::LowType spawnId; // our counter (bit 39 set)
        uint32 entry;
        uint32 mapId;
        float x, y, z, o;
        uint32 phaseId;
        uint32 respawnDelaySecs;
        uint32 spawntimesecs;
        float wander_distance;
        uint8 movementType;
        uint64 npcflag;              // 0 = template default
        uint32 unit_flags;           // 0 = template default
        uint32 unit_flags2;          // 0 = template default
        uint32 faction;              // 0 = template default
        uint8 level;                 // 0 = template default
        uint8 equipment_id;
        uint32 gossipMenuId;
        uint8 reactState;
        uint32 display_id;           // 0 = template default
        std::string scriptName;
        std::string stringId;
        bool isActive;

        // Optional runtime-only fields not in DB
        bool isPersisted = false;
    };

    struct ASPersistGameObjectSpawn
    {
        ObjectGuid guid;
        ObjectGuid::LowType spawnId;
        uint32 entry;
        uint32 mapId;
        float x, y, z, o;
        float rot0, rot1, rot2, rot3;
        uint32 phaseId;
        uint32 respawnDelaySecs;
        uint32 spawntimesecs;
        uint8 goState;
        uint8 animprogress;
        uint32 artKit;
        std::string scriptName;
        std::string stringId;
        bool isActive;

        bool isPersisted = false;
    };

    // ------------------------------------------------------------------------
    // ASSpawnDB — static helpers for AngelDB spawn CRUD
    // ------------------------------------------------------------------------
    class ASSpawnDB
    {
    public:
        // GUID separation constant
        static constexpr ObjectGuid::LowType AS_SPAWN_GUID_FLAG = UI64LIT(0x0000008000000000);

        // Check if a raw GUID value is from AngelCore (bit 39 set)
        static bool IsAngelCoreSpawn(ObjectGuid::LowType spawnId)
        {
            return (spawnId & AS_SPAWN_GUID_FLAG) != 0;
        }

        // Generate next spawn ID (counter + flag bit)
        static ObjectGuid::LowType GenerateCreatureSpawnId();
        static ObjectGuid::LowType GenerateGameObjectSpawnId();

        // --- CRUD for creature spawns ---
        static bool InsertCreatureSpawn(const ASPersistCreatureSpawn& spawn);
        static bool UpdateCreatureSpawn(const ASPersistCreatureSpawn& spawn);
        static bool DeleteCreatureSpawn(ObjectGuid::LowType spawnId);

        // --- CRUD for gameobject spawns ---
        static bool InsertGameObjectSpawn(const ASPersistGameObjectSpawn& spawn);
        static bool UpdateGameObjectSpawn(const ASPersistGameObjectSpawn& spawn);
        static bool DeleteGameObjectSpawn(ObjectGuid::LowType spawnId);

        // --- Bulk load from AngelDB ---
        static std::vector<ASPersistCreatureSpawn>   LoadAllCreatureSpawns();
        static std::vector<ASPersistGameObjectSpawn> LoadAllGameObjectSpawns();

        // --- Persist active spawns to DB (used on shutdown) ---
        static void MarkCreatureInactive(ObjectGuid::LowType spawnId);
        static void MarkGameObjectInactive(ObjectGuid::LowType spawnId);

        // --- Sync next-ID counters from DB ---
        static void SyncCountersFromDB();

    private:
        static std::atomic<ObjectGuid::LowType> s_nextCreatureSpawnId;
        static std::atomic<ObjectGuid::LowType> s_nextGameObjectSpawnId;
    };

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASSPAWNDB_H
