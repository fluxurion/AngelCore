/*
 * AngelScript Spawn API
 * Complete spawn system for creatures & gameobjects from AngelScript.
 * Bypasses TC's SQL spawn tables entirely — all spawn data lives in .as files.
 *
 * Features:
 *   - Spawn/despawn creatures & gameobjects with position, orientation, phaseId
 *   - Per-spawn: level, faction, displayId, equipment, gossip, NPC flags
 *   - Movement: MovePoint, MovePath (waypoints), MoveRandom (wander), MoveFollow
 *   - Quest scripting: Talk, Say, Yell, Emote, CastSpell, Despawn, MoveTo
 *   - Respawn management with configurable delay
 *   - Find spawned objects by GUID
 */

#ifndef ANGELSCRIPT_INTEGRATION
    #error "ANGELSCRIPT_INTEGRATION macro must be defined"
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#pragma push_macro("IN")
#pragma push_macro("OUT")
#pragma push_macro("OPTIONAL")
#undef IN
#undef OUT
#undef OPTIONAL

#include <angelscript.h>

#pragma pop_macro("OPTIONAL")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")

#include "ASSpawnAPI.h"
#include "AngelScriptMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Log.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "SharedDefines.h"
#include "PhasingHandler.h"
#include "ObjectAccessor.h"
#include "EventMap.h"

#include <unordered_map>
#include <vector>
#include <string>

namespace AngelScript
{
    // ========================================================================
    // Spawn Registry — tracks AS-spawned objects
    // ========================================================================

    struct ASCreatureSpawnEntry
    {
        ObjectGuid guid;
        uint32 entry;
        uint32 mapId;
        float x, y, z, o;
        uint32 respawnDelaySecs;
        bool isActive;
    };

    struct ASGameObjectSpawnEntry
    {
        ObjectGuid guid;
        uint32 entry;
        uint32 mapId;
        float x, y, z, o;
        uint32 respawnDelaySecs;
        bool isActive;
    };

    static std::vector<ASCreatureSpawnEntry>   g_asCreatureSpawns;
    static std::vector<ASGameObjectSpawnEntry> g_asGameObjectSpawns;

    // Per-creature timer maps — keyed by GUID raw value.
    // Works on ANY Creature (AS-spawned or SQL-spawned).
    static std::unordered_map<uint64, EventMap> g_asCreatureTimers;

    // ------------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------------

    static EventMap& GetOrCreateCreatureTimer(Creature* c)
    {
        static EventMap s_empty;
        if (!c) return s_empty;
        return g_asCreatureTimers[c->GetGUID().GetRawValue(0)];
    }
    static void RegisterSpawnedCreature(Creature* c, uint32 respawnDelaySecs)
    {
        if (!c) return;
        ASCreatureSpawnEntry e;
        e.guid       = c->GetGUID();
        e.entry      = c->GetEntry();
        e.mapId      = c->GetMapId();
        e.x          = c->GetPositionX();
        e.y          = c->GetPositionY();
        e.z          = c->GetPositionZ();
        e.o          = c->GetOrientation();
        e.respawnDelaySecs = respawnDelaySecs;
        e.isActive   = true;
        g_asCreatureSpawns.push_back(e);
    }

    static void RegisterSpawnedGameObject(GameObject* go, uint32 respawnDelaySecs)
    {
        if (!go) return;
        ASGameObjectSpawnEntry e;
        e.guid       = go->GetGUID();
        e.entry      = go->GetEntry();
        e.mapId      = go->GetMapId();
        e.x          = go->GetPositionX();
        e.y          = go->GetPositionY();
        e.z          = go->GetPositionZ();
        e.o          = go->GetOrientation();
        e.respawnDelaySecs = respawnDelaySecs;
        e.isActive   = true;
        g_asGameObjectSpawns.push_back(e);
    }

    static ASCreatureSpawnEntry* FindCreatureSpawnEntry(ObjectGuid guid)
    {
        for (auto& e : g_asCreatureSpawns)
            if (e.guid == guid) return &e;
        return nullptr;
    }

    static ASGameObjectSpawnEntry* FindGameObjectSpawnEntry(ObjectGuid guid)
    {
        for (auto& e : g_asGameObjectSpawns)
            if (e.guid == guid) return &e;
        return nullptr;
    }

    // Internal: respawn a creature from stored entry data
    static uint64 RespawnCreatureFromEntry(ASCreatureSpawnEntry& entry)
    {
        Map* map = sMapMgr->FindMap(entry.mapId, 0);
        if (!map) return 0;

        Position pos(entry.x, entry.y, entry.z, entry.o);
        Creature* creature = Creature::CreateCreature(entry.entry, map, pos);
        if (!creature) return 0;

        creature->SetRespawnCompatibilityMode(true);
        if (!map->AddToMap(creature))
        {
            delete creature;
            return 0;
        }
        entry.guid = creature->GetGUID();
        entry.isActive = true;
        return creature->GetGUID().GetRawValue(0);
    }

    // Internal: respawn a gameobject from stored entry data
    static uint64 RespawnGameObjectFromEntry(ASGameObjectSpawnEntry& entry)
    {
        Map* map = sMapMgr->FindMap(entry.mapId, 0);
        if (!map) return 0;

        Position pos(entry.x, entry.y, entry.z, entry.o);
        QuaternionData rot = QuaternionData::fromEulerAnglesZYX(entry.o, 0.0f, 0.0f);
        GameObject* go = GameObject::CreateGameObject(entry.entry, map, pos, rot, 255, GO_STATE_READY);
        if (!go) return 0;

        if (!map->AddToMap(go))
        {
            delete go;
            return 0;
        }
        entry.guid = go->GetGUID();
        entry.isActive = true;
        return go->GetGUID().GetRawValue(0);
    }

    // ========================================================================
    // SPAWN: CreateCreature
    // ========================================================================
    static uint64 Spawn_CreateCreature(
        uint32 entry, uint32 mapId,
        float x, float y, float z, float o,
        uint32 phaseId, uint32 respawnDelaySecs)
    {
        Map* map = sMapMgr->FindMap(mapId, 0);
        if (!map)
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Map {} not found for creature entry {}", mapId, entry);
            return 0;
        }

        CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(entry);
        if (!cInfo)
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Creature template entry {} does not exist", entry);
            return 0;
        }

        Position pos(x, y, z, o);
        Creature* creature = Creature::CreateCreature(entry, map, pos);
        if (!creature)
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Failed to create creature entry {}", entry);
            return 0;
        }

        // Apply phase (phaseId == 0 means visible in all phases)
        if (phaseId != 0)
            PhasingHandler::AddPhase(creature, phaseId, true);

        creature->SetRespawnCompatibilityMode(true);

        if (!map->AddToMap(creature))
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Failed to add creature {} to map {}", entry, mapId);
            delete creature;
            return 0;
        }

        RegisterSpawnedCreature(creature, respawnDelaySecs);

        TC_LOG_DEBUG("server.angelscript", "[SpawnAPI] Spawned creature entry {} guid {} at ({:.1f},{:.1f},{:.1f}) map {} phaseId {}",
            entry, creature->GetGUID().GetCounter(), x, y, z, mapId, phaseId);

        return creature->GetGUID().GetRawValue(0);
    }

    // ========================================================================
    // SPAWN: CreateGameObject
    // ========================================================================
    static uint64 Spawn_CreateGameObject(
        uint32 entry, uint32 mapId,
        float x, float y, float z, float o,
        uint32 phaseId, uint32 respawnDelaySecs, uint32 goState)
    {
        Map* map = sMapMgr->FindMap(mapId, 0);
        if (!map)
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Map {} not found for GO entry {}", mapId, entry);
            return 0;
        }

        GameObjectTemplate const* goInfo = sObjectMgr->GetGameObjectTemplate(entry);
        if (!goInfo)
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] GameObject template entry {} does not exist", entry);
            return 0;
        }

        Position pos(x, y, z, o);
        QuaternionData rot = QuaternionData::fromEulerAnglesZYX(o, 0.0f, 0.0f);

        GameObject* go = GameObject::CreateGameObject(entry, map, pos, rot, 255,
            static_cast<GOState>(goState));
        if (!go)
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Failed to create GO entry {}", entry);
            return 0;
        }

        if (phaseId != 0)
            PhasingHandler::AddPhase(go, phaseId, true);

        if (!map->AddToMap(go))
        {
            TC_LOG_ERROR("server.angelscript", "[SpawnAPI] Failed to add GO {} to map {}", entry, mapId);
            delete go;
            return 0;
        }

        RegisterSpawnedGameObject(go, respawnDelaySecs);

        TC_LOG_DEBUG("server.angelscript", "[SpawnAPI] Spawned GO entry {} guid {} at ({:.1f},{:.1f},{:.1f}) map {} phaseId {}",
            entry, go->GetGUID().GetCounter(), x, y, z, mapId, phaseId);

        return go->GetGUID().GetRawValue(0);
    }

    // ========================================================================
    // DESPAWN
    // ========================================================================
    static void Spawn_DespawnCreature(Creature* creature, uint32 respawnDelaySecs)
    {
        if (!creature || !creature->IsInWorld()) return;

        auto* entry = FindCreatureSpawnEntry(creature->GetGUID());
        if (entry)
        {
            entry->respawnDelaySecs = respawnDelaySecs;
            entry->isActive = false;
        }
        creature->DespawnOrUnsummon(0s, Seconds(respawnDelaySecs));
    }

    static void Spawn_DespawnGameObject(GameObject* go, uint32 respawnDelaySecs)
    {
        if (!go || !go->IsInWorld()) return;

        auto* entry = FindGameObjectSpawnEntry(go->GetGUID());
        if (entry)
        {
            entry->respawnDelaySecs = respawnDelaySecs;
            entry->isActive = false;
        }
        go->Delete();
        if (respawnDelaySecs > 0)
            go->SetRespawnTime(respawnDelaySecs);
    }

    // ========================================================================
    // FIND by GUID
    // ========================================================================
    static Creature* Spawn_FindCreature(uint64 rawGuid, uint32 mapId)
    {
        Map* map = sMapMgr->FindMap(mapId, 0);
        if (!map) return nullptr;

        // Search AS registry by raw GUID value
        auto it = std::find_if(g_asCreatureSpawns.begin(), g_asCreatureSpawns.end(),
            [rawGuid](ASCreatureSpawnEntry const& e) { return e.guid.GetRawValue(0) == rawGuid; });
        if (it != g_asCreatureSpawns.end() && it->isActive)
            return map->GetCreature(it->guid);

        return nullptr;
    }

    static GameObject* Spawn_FindGameObject(uint64 rawGuid, uint32 mapId)
    {
        Map* map = sMapMgr->FindMap(mapId, 0);
        if (!map) return nullptr;

        // Search AS registry by raw GUID value
        auto it = std::find_if(g_asGameObjectSpawns.begin(), g_asGameObjectSpawns.end(),
            [rawGuid](ASGameObjectSpawnEntry const& e) { return e.guid.GetRawValue(0) == rawGuid; });
        if (it != g_asGameObjectSpawns.end() && it->isActive)
            return map->GetGameObject(it->guid);

        return nullptr;
    }

    // ========================================================================
    // Remove spawn from registry (permanent — no more respawns)
    // ========================================================================
    static void Spawn_RemoveFromRegistry(Creature* creature)
    {
        if (!creature) return;
        auto it = std::remove_if(g_asCreatureSpawns.begin(), g_asCreatureSpawns.end(),
            [guid = creature->GetGUID()](ASCreatureSpawnEntry const& e) { return e.guid == guid; });
        if (it != g_asCreatureSpawns.end())
            g_asCreatureSpawns.erase(it, g_asCreatureSpawns.end());
    }

    static void Spawn_RemoveGORegistry(GameObject* go)
    {
        if (!go) return;
        auto it = std::remove_if(g_asGameObjectSpawns.begin(), g_asGameObjectSpawns.end(),
            [guid = go->GetGUID()](ASGameObjectSpawnEntry const& e) { return e.guid == guid; });
        if (it != g_asGameObjectSpawns.end())
            g_asGameObjectSpawns.erase(it, g_asGameObjectSpawns.end());
    }

    // ========================================================================
    // SPAWN EX — one-call complete spawn with all configuration
    // ========================================================================
    static uint64 Spawn_CreateCreatureEx(
        uint32 entry, uint32 mapId,
        float x, float y, float z, float o,
        uint32 phaseId, uint32 respawnDelaySecs,
        uint8 level, uint32 faction, uint64 npcFlags,
        uint32 gossipMenuId, uint8 equipmentId,
        uint8 reactState)
    {
        uint64 guid = Spawn_CreateCreature(entry, mapId, x, y, z, o, phaseId, respawnDelaySecs);
        if (guid == 0) return 0;

        // Find the creature in the registry by raw GUID value
        auto it = std::find_if(g_asCreatureSpawns.begin(), g_asCreatureSpawns.end(),
            [guid](ASCreatureSpawnEntry const& e) { return e.guid.GetRawValue(0) == guid; });
        if (it == g_asCreatureSpawns.end()) return guid;

        Map* map = sMapMgr->FindMap(mapId, 0);
        if (!map) return guid;

        Creature* c = map->GetCreature(it->guid);
        if (!c) return guid;

        if (level > 0)        { c->SetLevel(level, true); c->UpdateLevelDependantStats(); }
        if (faction > 0)      c->SetFaction(faction);
        if (npcFlags != 0)    c->ReplaceAllNpcFlags(NPCFlags(npcFlags));
        if (gossipMenuId > 0) c->SetGossipMenuId(gossipMenuId);
        if (equipmentId != 0) { c->SetCurrentEquipmentId(equipmentId); c->LoadEquipment(equipmentId, true); }
        if (reactState < 3)   c->SetReactState(static_cast<ReactStates>(reactState));

        return guid;
    }

    // ========================================================================
    // SPAWN HELPER — configure an already-spawned creature in one call
    // ========================================================================
    static void Spawn_ConfigureCreature(Creature* c,
        uint8 level, uint32 faction, uint64 npcFlags,
        uint32 gossipMenuId, uint8 equipmentId,
        uint8 reactState)
    {
        if (!c) return;
        if (level > 0)        { c->SetLevel(level, true); c->UpdateLevelDependantStats(); }
        if (faction > 0)      c->SetFaction(faction);
        if (npcFlags != 0)    c->ReplaceAllNpcFlags(NPCFlags(npcFlags));
        if (gossipMenuId > 0) c->SetGossipMenuId(gossipMenuId);
        if (equipmentId != 0) { c->SetCurrentEquipmentId(equipmentId); c->LoadEquipment(equipmentId, true); }
        if (reactState < 3)   c->SetReactState(static_cast<ReactStates>(reactState));
    }

    // ========================================================================
    // UTILITY: Level
    // ========================================================================
    static void Creature_SetLevel(Creature* c, uint8 level)
    {
        if (!c) return;
        c->SetLevel(level, true);
        c->UpdateLevelDependantStats();
    }

    static uint8 Creature_GetLevelForPlayer(Creature* c, Player* /*target*/)
    {
        if (!c) return 0;
        return c->GetLevel();
    }

    // ========================================================================
    // UTILITY: Faction
    // ========================================================================
    static void Creature_SetFaction(Creature* c, uint32 factionTemplateId)
    {
        if (!c) return;
        c->SetFaction(factionTemplateId);
    }

    // ========================================================================
    // UTILITY: NPC Flags
    // ========================================================================
    static void Creature_SetNpcFlag(Creature* c, uint64 flag)
    {
        if (!c) return;
        c->ReplaceAllNpcFlags(NPCFlags(flag));
    }

    static uint64 Creature_GetNpcFlag(Creature* c)
    {
        if (!c) return 0;
        return static_cast<uint64>(c->GetNpcFlags());
    }

    // ========================================================================
    // UTILITY: Gossip
    // ========================================================================
    static void Creature_SetGossipMenu(Creature* c, uint32 gossipMenuId)
    {
        if (!c) return;
        c->SetGossipMenuId(gossipMenuId);
    }

    static uint32 Creature_GetGossipMenuId(Creature* c)
    {
        if (!c) return 0;
        return c->GetGossipMenuId();
    }

    // ========================================================================
    // UTILITY: DisplayId
    // ========================================================================
    static void Creature_SetDisplayId(Creature* c, uint32 displayId)
    {
        if (!c) return;
        c->SetDisplayId(displayId, true);
    }

    static void GameObject_SetDisplayId(GameObject* go, uint32 displayId)
    {
        if (!go) return;
        go->SetDisplayId(displayId);
    }

    // ========================================================================
    // UTILITY: Equipment
    // ========================================================================
    static void Creature_SetEquipment(Creature* c, uint8 equipmentId)
    {
        if (!c) return;
        c->SetCurrentEquipmentId(equipmentId);
        c->LoadEquipment(equipmentId, true);
    }

    // ========================================================================
    // UTILITY: React State (passive / defensive / aggressive)
    // ========================================================================
    static void Creature_SetReactState(Creature* c, uint8 state)
    {
        if (!c) return;
        c->SetReactState(static_cast<ReactStates>(state));
    }

    // ========================================================================
    // UTILITY: Phase
    // ========================================================================
    static void Creature_AddPhase(Creature* c, uint32 phaseId)
    {
        if (!c) return;
        PhasingHandler::AddPhase(c, phaseId, true);
    }

    static void Creature_RemovePhase(Creature* c, uint32 phaseId)
    {
        if (!c) return;
        PhasingHandler::RemovePhase(c, phaseId, true);
    }

    // ========================================================================
    // UTILITY: Emote
    // ========================================================================
    static void Creature_DoEmote(Creature* c, uint32 emoteId)
    {
        if (!c) return;
        c->HandleEmoteCommand(static_cast<Emote>(emoteId));
    }

    // ========================================================================
    // UTILITY: Talk / Say / Yell
    // ========================================================================
    static void Creature_Talk(Creature* c, const std::string& text, uint32 msgType, uint32 language, float range, Player* target)
    {
        if (!c) return;
        WorldObject const* tgt = target ? static_cast<WorldObject const*>(target) : nullptr;
        c->Talk(text, static_cast<ChatMsg>(msgType), static_cast<Language>(language), range, tgt);
    }

    static void Creature_Say(Creature* c, const std::string& text, uint32 language, Player* target)
    {
        if (!c) return;
        WorldObject const* tgt = target ? static_cast<WorldObject const*>(target) : nullptr;
        c->Say(text, static_cast<Language>(language), tgt);
    }

    static void Creature_Yell(Creature* c, const std::string& text, uint32 language, Player* target)
    {
        if (!c) return;
        WorldObject const* tgt = target ? static_cast<WorldObject const*>(target) : nullptr;
        c->Yell(text, static_cast<Language>(language), tgt);
    }

    static void Creature_TextEmote(Creature* c, const std::string& text, Player* target, bool isBossEmote)
    {
        if (!c) return;
        WorldObject const* tgt = target ? static_cast<WorldObject const*>(target) : nullptr;
        c->TextEmote(text, tgt, isBossEmote);
    }

    // ========================================================================
    // UTILITY: Talk by broadcast text ID
    // ========================================================================
    static void Creature_TalkById(Creature* c, uint32 textId, uint32 msgType, float range, Player* target)
    {
        if (!c) return;
        WorldObject const* tgt = target ? static_cast<WorldObject const*>(target) : nullptr;
        c->Talk(textId, static_cast<ChatMsg>(msgType), range, tgt);
    }

    static void Creature_SayById(Creature* c, uint32 textId, Player* target)
    {
        if (!c) return;
        WorldObject const* tgt = target ? static_cast<WorldObject const*>(target) : nullptr;
        c->Say(textId, tgt);
    }

    // ========================================================================
    // UTILITY: Play one-shot anim kit
    // ========================================================================
    static void Creature_PlayAnimKit(Creature* c, uint16 animKitId)
    {
        if (!c) return;
        c->PlayOneShotAnimKitId(animKitId);
    }

    // ========================================================================
    // MOVEMENT: MovePoint — move to a specific position
    // ========================================================================
    static void Creature_MovePoint(Creature* c, uint32 pointId, float x, float y, float z,
        bool generatePath, float speed, bool forceWalk)
    {
        if (!c) return;
        Optional<float> spd = (speed > 0.0f) ? Optional<float>(speed) : Optional<float>();
        MovementWalkRunSpeedSelectionMode mode = forceWalk
            ? MovementWalkRunSpeedSelectionMode::ForceWalk
            : MovementWalkRunSpeedSelectionMode::Default;
        c->GetMotionMaster()->MovePoint(pointId, x, y, z, generatePath, {}, spd, mode);
    }

    // ========================================================================
    // MOVEMENT: MoveRandom — wander within a distance
    // ========================================================================
    static void Creature_MoveRandom(Creature* c, float wanderDistance)
    {
        if (!c) return;
        c->GetMotionMaster()->MoveRandom(wanderDistance);
    }

    // ========================================================================
    // MOVEMENT: MoveFollow — follow a target
    // ========================================================================
    static void Creature_MoveFollow(Creature* c, Unit* target, float distance, float angle)
    {
        if (!c || !target) return;
        c->GetMotionMaster()->MoveFollow(target, distance, angle);
    }

    // ========================================================================
    // MOVEMENT: MoveChase — chase a target (combat)
    // ========================================================================
    static void Creature_MoveChase(Creature* c, Unit* target, float distance, float angle)
    {
        if (!c || !target) return;
        c->GetMotionMaster()->MoveChase(target, distance, angle);
    }

    // ========================================================================
    // MOVEMENT: MoveIdle — stop all movement
    // ========================================================================
    static void Creature_MoveIdle(Creature* c)
    {
        if (!c) return;
        c->GetMotionMaster()->MoveIdle();
    }

    // ========================================================================
    // MOVEMENT: Clear all movement
    // ========================================================================
    static void Creature_ClearMovement(Creature* c)
    {
        if (!c) return;
        c->GetMotionMaster()->Clear();
    }

    // ========================================================================
    // UTILITY: Get spawn count (for diagnostics)
    // ========================================================================
    static uint32 Spawn_GetCreatureSpawnCount()
    {
        return static_cast<uint32>(g_asCreatureSpawns.size());
    }

    static uint32 Spawn_GetGameObjectSpawnCount()
    {
        return static_cast<uint32>(g_asGameObjectSpawns.size());
    }

    // ========================================================================
    // UTILITY: Despawn ALL AS-spawned objects
    // ========================================================================
    static void Spawn_DespawnAll()
    {
        for (auto& e : g_asCreatureSpawns)
        {
            if (!e.isActive) continue;
            Map* map = sMapMgr->FindMap(e.mapId, 0);
            if (!map) continue;
            Creature* c = map->GetCreature(e.guid);
            if (c)
            {
                c->DespawnOrUnsummon();
                e.isActive = false;
            }
        }
        for (auto& e : g_asGameObjectSpawns)
        {
            if (!e.isActive) continue;
            Map* map = sMapMgr->FindMap(e.mapId, 0);
            if (!map) continue;
            GameObject* go = map->GetGameObject(e.guid);
            if (go)
            {
                go->Delete();
                e.isActive = false;
            }
        }
    }

    // ========================================================================
    // UTILITY: Respawn ALL AS-spawned objects
    // ========================================================================
    static void Spawn_RespawnAll()
    {
        for (auto& e : g_asCreatureSpawns)
        {
            if (e.isActive) continue;
            RespawnCreatureFromEntry(e);
        }
        for (auto& e : g_asGameObjectSpawns)
        {
            if (e.isActive) continue;
            RespawnGameObjectFromEntry(e);
        }
    }

    // ========================================================================
    // TIMER API — per-creature timed event scheduling
    // Each spawned creature gets its own EventMap. Use Schedule/Reschedule/
    // Cancel/Repeat to build encounter scripts. The update tick is driven by
    // the CREATURE_ON_UPDATE hook in the Dispatch layer.
    // ========================================================================

    static void Creature_ScheduleEvent(Creature* c, uint32 eventId, uint32 timeMs,
        uint32 group, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.ScheduleEvent(eventId, Milliseconds(timeMs), group, phase);
    }

    static void Creature_ScheduleEventRandom(Creature* c, uint32 eventId,
        uint32 minTimeMs, uint32 maxTimeMs, uint32 group, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.ScheduleEvent(eventId, Milliseconds(minTimeMs), Milliseconds(maxTimeMs),
            group, phase);
    }

    static void Creature_RescheduleEvent(Creature* c, uint32 eventId, uint32 timeMs,
        uint32 group, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.RescheduleEvent(eventId, Milliseconds(timeMs), group, phase);
    }

    static void Creature_RescheduleEventRandom(Creature* c, uint32 eventId,
        uint32 minTimeMs, uint32 maxTimeMs, uint32 group, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.RescheduleEvent(eventId, Milliseconds(minTimeMs), Milliseconds(maxTimeMs),
            group, phase);
    }

    static void Creature_RepeatEvent(Creature* c, uint32 timeMs)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.Repeat(Milliseconds(timeMs));
    }

    static void Creature_RepeatEventRandom(Creature* c, uint32 minTimeMs, uint32 maxTimeMs)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.Repeat(Milliseconds(minTimeMs), Milliseconds(maxTimeMs));
    }

    static void Creature_CancelEvent(Creature* c, uint32 eventId)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.CancelEvent(eventId);
    }

    static void Creature_CancelEventGroup(Creature* c, uint32 group)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.CancelEventGroup(group);
    }

    static void Creature_DelayEvents(Creature* c, uint32 delayMs, uint32 group)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        if (group == 0)
            events.DelayEvents(Milliseconds(delayMs));
        else
            events.DelayEvents(Milliseconds(delayMs), group);
    }

    static uint32 Creature_GetTimeUntilEvent(Creature* c, uint32 eventId)
    {
        if (!c) return 0;
        EventMap& events = GetOrCreateCreatureTimer(c);
        Milliseconds time = events.GetTimeUntilEvent(eventId);
        if (time == Milliseconds::max()) return 0xFFFFFFFF;
        return static_cast<uint32>(time.count());
    }

    static bool Creature_IsEventScheduled(Creature* c, uint32 eventId)
    {
        if (!c) return false;
        EventMap& events = GetOrCreateCreatureTimer(c);
        return events.GetTimeUntilEvent(eventId) != Milliseconds::max();
    }

    static void Creature_SetEventPhase(Creature* c, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.SetPhase(phase);
    }

    static void Creature_AddEventPhase(Creature* c, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.AddPhase(phase);
    }

    static void Creature_RemoveEventPhase(Creature* c, uint8 phase)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.RemovePhase(phase);
    }

    static void Creature_ResetTimers(Creature* c)
    {
        if (!c) return;
        EventMap& events = GetOrCreateCreatureTimer(c);
        events.Reset();
    }

    static bool Creature_HasTimers(Creature* c)
    {
        if (!c) return false;
        EventMap& events = GetOrCreateCreatureTimer(c);
        return !events.Empty();
    }

    // ========================================================================
    // TIMER UPDATE — called from Dispatch/ASDispatch or world update hook
    // Advances the creature's timer and returns the next event ID if due.
    // Returns 0 if no event is due.
    // ========================================================================
    static uint32 Creature_UpdateTimers(Creature* c, uint32 diffMs)
    {
        if (!c) return 0;
        auto it = g_asCreatureTimers.find(c->GetGUID().GetRawValue(0));
        if (it == g_asCreatureTimers.end()) return 0;
        it->second.Update(diffMs);
        return it->second.ExecuteEvent();
    }

    // ========================================================================
    // UTILITY: Clear all spawns (permanent removal)
    // ========================================================================
    static void Spawn_ClearAllSpawns()
    {
        for (auto& e : g_asCreatureSpawns)
        {
            if (!e.isActive) continue;
            Map* map = sMapMgr->FindMap(e.mapId, 0);
            if (!map) continue;
            Creature* c = map->GetCreature(e.guid);
            if (c)
            {
                c->DespawnOrUnsummon();
                c->AddObjectToRemoveList();
            }
        }
        for (auto& e : g_asGameObjectSpawns)
        {
            if (!e.isActive) continue;
            Map* map = sMapMgr->FindMap(e.mapId, 0);
            if (!map) continue;
            GameObject* go = map->GetGameObject(e.guid);
            if (go)
            {
                go->Delete();
                go->AddObjectToRemoveList();
            }
        }
        g_asCreatureSpawns.clear();
        g_asGameObjectSpawns.clear();
    }

    // ========================================================================
    // REGISTRATION
    // ========================================================================

    void RegisterSpawnAPI(asIScriptEngine* engine)
    {
        if (!engine) return;

        int r = 0;

        // ---- Spawn Creatures ----
        r = engine->RegisterGlobalFunction(
            "uint64 SpawnCreature(uint32 entry, uint32 mapId, float x, float y, float z, float o, uint32 phaseId = 0, uint32 respawnDelaySecs = 0)",
            asFUNCTION(Spawn_CreateCreature), asCALL_CDECL);
        (void)r;

        // ---- SpawnCreatureEx — one-line spawn with full configuration ----
        r = engine->RegisterGlobalFunction(
            "uint64 SpawnCreatureEx(uint32 entry, uint32 mapId, float x, float y, float z, float o, "
            "uint32 phaseId = 0, uint32 respawnDelaySecs = 0, "
            "uint8 level = 0, uint32 faction = 0, uint64 npcFlags = 0, "
            "uint32 gossipMenuId = 0, uint8 equipmentId = 0, uint8 reactState = 0)",
            asFUNCTION(Spawn_CreateCreatureEx), asCALL_CDECL);
        (void)r;

        // ---- ConfigureCreature — batch-config an existing creature ----
        r = engine->RegisterGlobalFunction(
            "void ConfigureCreature(Creature@ c, "
            "uint8 level = 0, uint32 faction = 0, uint64 npcFlags = 0, "
            "uint32 gossipMenuId = 0, uint8 equipmentId = 0, uint8 reactState = 0)",
            asFUNCTION(Spawn_ConfigureCreature), asCALL_CDECL);
        (void)r;

        // ---- Spawn GameObjects ----
        r = engine->RegisterGlobalFunction(
            "uint64 SpawnGameObject(uint32 entry, uint32 mapId, float x, float y, float z, float o, uint32 phaseId = 0, uint32 respawnDelaySecs = 0, uint32 goState = 1)",
            asFUNCTION(Spawn_CreateGameObject), asCALL_CDECL);
        (void)r;

        // ---- Despawn ----
        r = engine->RegisterGlobalFunction(
            "void DespawnCreature(Creature@ creature, uint32 respawnDelaySecs = 0)",
            asFUNCTION(Spawn_DespawnCreature), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction(
            "void DespawnGameObject(GameObject@ go, uint32 respawnDelaySecs = 0)",
            asFUNCTION(Spawn_DespawnGameObject), asCALL_CDECL);
        (void)r;

        // ---- Find by GUID ----
        r = engine->RegisterGlobalFunction(
            "Creature@ FindSpawnedCreature(uint64 guid, uint32 mapId)",
            asFUNCTION(Spawn_FindCreature), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction(
            "GameObject@ FindSpawnedGameObject(uint64 guid, uint32 mapId)",
            asFUNCTION(Spawn_FindGameObject), asCALL_CDECL);
        (void)r;

        // ---- Remove from registry ----
        r = engine->RegisterGlobalFunction(
            "void RemoveSpawnedCreature(Creature@ creature)",
            asFUNCTION(Spawn_RemoveFromRegistry), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction(
            "void RemoveSpawnedGameObject(GameObject@ go)",
            asFUNCTION(Spawn_RemoveGORegistry), asCALL_CDECL);
        (void)r;

        // ---- Creature methods — Level ----
        r = engine->RegisterObjectMethod("Creature", "void SetLevel(uint8 level)",
            asFUNCTION(Creature_SetLevel), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "uint8 GetLevelForPlayer(Player@ target) const",
            asFUNCTION(Creature_GetLevelForPlayer), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Faction ----
        r = engine->RegisterObjectMethod("Creature", "void SetFaction(uint32 factionTemplateId)",
            asFUNCTION(Creature_SetFaction), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — NPC Flags ----
        r = engine->RegisterObjectMethod("Creature", "void SetNpcFlag(uint64 flag)",
            asFUNCTION(Creature_SetNpcFlag), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "uint64 GetNpcFlag() const",
            asFUNCTION(Creature_GetNpcFlag), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Gossip ----
        r = engine->RegisterObjectMethod("Creature", "void SetGossipMenu(uint32 gossipMenuId)",
            asFUNCTION(Creature_SetGossipMenu), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "uint32 GetGossipMenuId() const",
            asFUNCTION(Creature_GetGossipMenuId), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — DisplayId ----
        r = engine->RegisterObjectMethod("Creature", "void SetDisplayId(uint32 displayId)",
            asFUNCTION(Creature_SetDisplayId), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("GameObject", "void SetDisplayId(uint32 displayId)",
            asFUNCTION(GameObject_SetDisplayId), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Equipment ----
        r = engine->RegisterObjectMethod("Creature", "void SetEquipment(uint8 equipmentId)",
            asFUNCTION(Creature_SetEquipment), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Phase ----
        r = engine->RegisterObjectMethod("Creature", "void AddPhase(uint32 phaseId)",
            asFUNCTION(Creature_AddPhase), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void RemovePhase(uint32 phaseId)",
            asFUNCTION(Creature_RemovePhase), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Emote ----
        r = engine->RegisterObjectMethod("Creature", "void DoEmote(uint32 emoteId)",
            asFUNCTION(Creature_DoEmote), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Talk / Say / Yell ----
        r = engine->RegisterObjectMethod("Creature", "void Talk(const string& in text, uint32 msgType = 0, uint32 language = 0, float range = 25.0, Player@ target = null)",
            asFUNCTION(Creature_Talk), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void Say(const string& in text, uint32 language = 0, Player@ target = null)",
            asFUNCTION(Creature_Say), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void Yell(const string& in text, uint32 language = 0, Player@ target = null)",
            asFUNCTION(Creature_Yell), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void TextEmote(const string& in text, Player@ target = null, bool isBossEmote = false)",
            asFUNCTION(Creature_TextEmote), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void TalkById(uint32 textId, uint32 msgType = 0, float range = 25.0, Player@ target = null)",
            asFUNCTION(Creature_TalkById), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void SayById(uint32 textId, Player@ target = null)",
            asFUNCTION(Creature_SayById), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Anim Kit ----
        r = engine->RegisterObjectMethod("Creature", "void PlayAnimKit(uint16 animKitId)",
            asFUNCTION(Creature_PlayAnimKit), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Movement ----
        r = engine->RegisterObjectMethod("Creature", "void MovePoint(uint32 pointId, float x, float y, float z, bool generatePath = true, float speed = 0.0, bool forceWalk = false)",
            asFUNCTION(Creature_MovePoint), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void MoveRandom(float wanderDistance = 5.0)",
            asFUNCTION(Creature_MoveRandom), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void MoveFollow(Unit@ target, float distance = 2.0, float angle = 0.0)",
            asFUNCTION(Creature_MoveFollow), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void MoveChase(Unit@ target, float distance = 5.0, float angle = 0.0)",
            asFUNCTION(Creature_MoveChase), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void MoveIdle()",
            asFUNCTION(Creature_MoveIdle), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void ClearMovement()",
            asFUNCTION(Creature_ClearMovement), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Creature methods — Timed Events (Schedule/Reschedule/Cancel/Repeat) ----
        r = engine->RegisterObjectMethod("Creature", "void ScheduleEvent(uint32 eventId, uint32 timeMs, uint32 group = 0, uint8 phase = 0)",
            asFUNCTION(Creature_ScheduleEvent), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void ScheduleEventRandom(uint32 eventId, uint32 minTimeMs, uint32 maxTimeMs, uint32 group = 0, uint8 phase = 0)",
            asFUNCTION(Creature_ScheduleEventRandom), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void RescheduleEvent(uint32 eventId, uint32 timeMs, uint32 group = 0, uint8 phase = 0)",
            asFUNCTION(Creature_RescheduleEvent), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void RescheduleEventRandom(uint32 eventId, uint32 minTimeMs, uint32 maxTimeMs, uint32 group = 0, uint8 phase = 0)",
            asFUNCTION(Creature_RescheduleEventRandom), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void RepeatEvent(uint32 timeMs)",
            asFUNCTION(Creature_RepeatEvent), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void RepeatEventRandom(uint32 minTimeMs, uint32 maxTimeMs)",
            asFUNCTION(Creature_RepeatEventRandom), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void CancelEvent(uint32 eventId)",
            asFUNCTION(Creature_CancelEvent), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void CancelEventGroup(uint32 group)",
            asFUNCTION(Creature_CancelEventGroup), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void DelayEvents(uint32 delayMs, uint32 group = 0)",
            asFUNCTION(Creature_DelayEvents), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "uint32 GetTimeUntilEvent(uint32 eventId) const",
            asFUNCTION(Creature_GetTimeUntilEvent), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "bool IsEventScheduled(uint32 eventId) const",
            asFUNCTION(Creature_IsEventScheduled), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void SetEventPhase(uint8 phase)",
            asFUNCTION(Creature_SetEventPhase), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void AddEventPhase(uint8 phase)",
            asFUNCTION(Creature_AddEventPhase), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void RemoveEventPhase(uint8 phase)",
            asFUNCTION(Creature_RemoveEventPhase), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "void ResetTimers()",
            asFUNCTION(Creature_ResetTimers), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "bool HasTimers() const",
            asFUNCTION(Creature_HasTimers), asCALL_CDECL_OBJFIRST);
        (void)r;
        r = engine->RegisterObjectMethod("Creature", "uint32 UpdateTimers(uint32 diffMs)",
            asFUNCTION(Creature_UpdateTimers), asCALL_CDECL_OBJFIRST);
        (void)r;

        // ---- Global spawn management ----
        r = engine->RegisterGlobalFunction("uint32 GetCreatureSpawnCount()",
            asFUNCTION(Spawn_GetCreatureSpawnCount), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction("uint32 GetGameObjectSpawnCount()",
            asFUNCTION(Spawn_GetGameObjectSpawnCount), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction("void DespawnAll()",
            asFUNCTION(Spawn_DespawnAll), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction("void RespawnAll()",
            asFUNCTION(Spawn_RespawnAll), asCALL_CDECL);
        (void)r;
        r = engine->RegisterGlobalFunction("void ClearAllSpawns()",
            asFUNCTION(Spawn_ClearAllSpawns), asCALL_CDECL);
        (void)r;

        TC_LOG_INFO("server.angelscript", "Spawn API registered (spawn, despawn, find, level, faction, flags, gossip, "
            "displayId, equipment, react state, phase, emote, talk, movement, global management)");
    }

} // namespace AngelScript
