/*
 * Bulk Zone Spawns — Simple Spawning Example
 *
 * Three ways to spawn, from simplest to most flexible:
 *
 *   1. SpawnCreature()         — bare minimum: entry, position, phase
 *   2. SpawnCreatureEx()       — one call: position + level + faction + flags + gossip + equipment
 *   3. SpawnCreature() +       — spawn then chain-configure for full control
 *      creature.SetLevel() etc.
 *
 * Top-level functions for mass spawns:
 *   ConfigureCreature()        — batch-config an existing creature in one call
 *   ScheduleEvent()            — timed events on any creature
 */

#include "../includes/ScriptFramework.as"

// ========================================================================
// Style 1: Simple one-liners — just entry, pos, phase, respawn
// ========================================================================
void SpawnGoldshireGuards()
{
    // A guard — auto-leveled by TC, uses template faction, default everything
    SpawnCreature(1423, 0, -9464, 62, 56, 1.5, 169, 300);
    SpawnCreature(1423, 0, -9450, 55, 56, 3.0, 169, 300);
    SpawnCreature(1423, 0, -9470, 70, 56, 0.5, 169, 300);

    Print("[Guards] 3 Stormwind Guards spawned in Goldshire");
}

// ========================================================================
// Style 2: SpawnCreatureEx — everything in one call
// ========================================================================
void SpawnQuestNPCs()
{
    // Quest giver: level 60, friendly faction, gossip, vendor flag, equipment 2
    SpawnCreatureEx(123456, 0, -9460, 50, 56, 2.0,
        169,    // phaseId
        0,      // respawn (0 = never)
        60,     // level
        35,     // faction (friendly)
        0x81,   // npcFlags = GOSSIP | QUESTGIVER
        50001,  // gossip menu
        2,      // equipment set
        0);     // react state (passive)

    // Quest boss: level 63, hostile, no gossip, equipment 1, aggressive
    SpawnCreatureEx(123457, 0, -8960, -140, 84, 0.0,
        169,    // phaseId
        600,    // respawn 10 min
        63,     // level
        14,     // faction (hostile)
        0,      // no npc flags
        0,      // no gossip
        1,      // equipment
        2);     // aggressive

    Print("[Quests] Quest NPCs spawned");
}

// ========================================================================
// Style 3: Spawn + ConfigureCreature + timer — for scripted NPCs
// ========================================================================
void SpawnPatrolCaptain()
{
    uint64 guid = SpawnCreature(123458, 0, -8950, -130, 83, 1.2, 169, 300);

    Creature@ cap = FindSpawnedCreature(guid, 0);
    if (cap is null) return;

    // Batch-configure in one call
    ConfigureCreature(cap, 55, 11, 0x81, 50002, 3, 0 /* passive */);

    // Start patrol timer
    cap.ScheduleEvent(1, 3000);  // event 1 fires in 3 seconds

    Print("[Patrol] Captain spawned and configured");
}

// ========================================================================
// Example: spawning gameobjects
// ========================================================================
void SpawnWorldObjects()
{
    // Campfire
    SpawnGameObject(17900, 0, -9460, 60, 55, 0, 169, 0, 1);

    // Mailbox
    SpawnGameObject(32348, 0, -9455, 50, 56, 2.0, 169, 0, 1);

    // Chair
    SpawnGameObject(17902, 0, -9462, 58, 55, 1.0, 169, 0, 1);

    Print("[World] 3 gameobjects spawned");
}

// ========================================================================
// Example: spawning in different phases — only visible to quest-holders
// ========================================================================
void SpawnPhasedEnemies()
{
    // These enemies are only visible in phase 170 (quest phase)
    SpawnCreatureEx(123457, 0, -8970, -150, 84, 1.0,
        170, 600, 63, 14, 0, 0, 1, 2);  // phase 170

    SpawnCreatureEx(123457, 0, -8965, -145, 84, 2.0,
        170, 600, 63, 14, 0, 0, 1, 2);

    SpawnCreatureEx(123457, 0, -8975, -155, 84, 3.0,
        170, 600, 63, 14, 0, 0, 1, 2);

    Print("[Phased] 3 enemies spawned in phase 170");
}

// ========================================================================
// Example: large batch spawning from data arrays
// ========================================================================
void SpawnFromArray()
{
    // Define spawn data inline as array-of-arrays:
    // { entry, x, y, z, o, phaseId, respawn, level, faction, npcFlags, gossip, equip, react }
    array<array<int>> spawns = {
        // entry   x       y      z   o    phase  resp  lvl  fac  flags  goss  eq  react
        { 1423,   -9464,  62,    56, 150, 169,   300,  55,  11,  0x81,  0,    0,  1    },
        { 1423,   -9450,  55,    56, 300, 169,   300,  55,  11,  0x81,  0,    0,  1    },
        { 1423,   -9470,  70,    56, 50,  169,   300,  55,  11,  0x81,  0,    0,  1    },
        { 123456, -9460,  50,    56, 200, 169,   0,    60,  35,  0x81,  50001,2,  0   },
        { 123457, -8960,  -140,  84, 0,   169,   600,  63,  14,  0,     0,    1,  2   }
    };

    for (uint i = 0; i < spawns.length(); i++)
    {
        array<int>@ s = spawns[i];
        SpawnCreatureEx(
            s[0], 0,           // entry, mapId
            float(s[1])/100, float(s[2])/100, float(s[3])/100, float(s[4])/100,  // x,y,z,o
            s[5], s[6],        // phaseId, respawn
            s[7], s[8], s[9],  // level, faction, npcFlags
            s[10], s[11], s[12] // gossip, equip, react
        );
    }

    Print("[Batch] " + spawns.length() + " creatures spawned from data array");
}

// ========================================================================
// Hook registration
// ========================================================================
void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @SpawnGoldshireGuards);
    RegisterWorldScript(WORLD_ON_STARTUP, @SpawnQuestNPCs);
    RegisterWorldScript(WORLD_ON_STARTUP, @SpawnWorldObjects);
    RegisterWorldScript(WORLD_ON_STARTUP, @SpawnPhasedEnemies);
    // RegisterWorldScript(WORLD_ON_STARTUP, @SpawnFromArray);  // alternative approach

    Print("[BulkSpawn] All spawn hooks registered");
}
