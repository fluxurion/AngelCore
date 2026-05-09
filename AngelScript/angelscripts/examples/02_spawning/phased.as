/*
 * Example 02: Phased Spawning
 * Spawn NPCs in different phases. Players only see them when in the right phase.
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    uint32 PHASE_NORMAL = 169;

    // These NPCs are visible to everyone
    SpawnCreature(1423, 0, -9460, 62, 56, 1.5, PHASE_NORMAL);

    // These enemies only appear to players in phase 170 (quest phase)
    uint32 PHASE_QUEST = 170;
    SpawnCreatureEx(123457, 0, -8970, -150, 84, 1.0, PHASE_QUEST, 600, 63, 14, 0, 0, 1, 2);
    SpawnCreatureEx(123457, 0, -8965, -145, 84, 2.0, PHASE_QUEST, 600, 63, 14, 0, 0, 1, 2);
    SpawnCreatureEx(123457, 0, -8975, -155, 84, 3.0, PHASE_QUEST, 600, 63, 14, 0, 0, 1, 2);

    Print("Phased spawns ready: 1 normal guard, 3 quest-phase enemies");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
