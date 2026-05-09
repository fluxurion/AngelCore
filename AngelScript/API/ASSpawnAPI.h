/*
 * AngelScript Spawn API
 * Complete spawn system for creatures and gameobjects from AngelScript.
 * Bypasses TC's SQL spawn tables entirely — all spawn data lives in .as files.
 *
 * Features:
 *   - Spawn/despawn creatures & gameobjects with full position/rotation/phase control
 *   - Per-spawn level, faction, equipment, gossip menu, NPC flags
 *   - Movement control (waypoints, wander distance)
 *   - Quest scripting support (talk, emote, cast spell, move to point, despawn)
 *   - Respawn management
 *   - Phase-aware spawning (phaseId via PhasingHandler)
 */

#ifndef ASSPAWNAPI_H
#define ASSPAWNAPI_H

#ifdef ANGELSCRIPT_INTEGRATION

class asIScriptEngine;

namespace AngelScript
{
    void RegisterSpawnAPI(asIScriptEngine* engine);
}

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASSPAWNAPI_H
