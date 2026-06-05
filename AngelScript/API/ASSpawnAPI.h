/*
 * AngelScript Spawn API
 * Complete spawn system for creatures and gameobjects from AngelScript.
 * Bypasses TC's SQL spawn tables entirely — all spawn data lives in .as files.
 *
 * NEW: Automatic AngelDB persistence.
 *   All spawns are persisted to angelcore_db (as_creature_spawns / as_gameobject_spawns)
 *   using an isolated GUID range (bit 39 set) to avoid collision with TC spawns.
 *   On startup, persisted spawns are reloaded automatically.
 *
 * Features:
 *   - Spawn/despawn creatures & gameobjects with full position/rotation/phase control
 *   - Per-spawn level, faction, equipment, gossip menu, NPC flags
 *   - Movement control (waypoints, wander distance)
 *   - Quest scripting support (talk, emote, cast spell, move to point, despawn)
 *   - Respawn management
 *   - Phase-aware spawning (phaseId via PhasingHandler)
 *   - Automatic AngelDB persistence (opt-out via persist=false)
 */

#ifndef ASSPAWNAPI_H
#define ASSPAWNAPI_H

#ifdef ANGELSCRIPT_INTEGRATION

class asIScriptEngine;

namespace AngelScript
{
    // Register the spawn API with AngelScript engine.
    // Also triggers auto-load of persisted spawns from AngelDB.
    void RegisterSpawnAPI(asIScriptEngine* engine);

    // Load all persisted spawns from AngelDB (called automatically from RegisterSpawnAPI)
    void LoadPersistedSpawns();

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASSPAWNAPI_H
