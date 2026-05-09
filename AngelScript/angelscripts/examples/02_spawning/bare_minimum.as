/*
 * Example 02: Bare Minimum Spawn
 * SpawnCreature with only required args. Uses template defaults.
 *
 * Signature: SpawnCreature(entry, mapId, x, y, z, o, phaseId=0, respawnDelaySecs=0)
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    // A guard in Goldshire — level/faction/equipment from creature_template
    SpawnCreature(1423, 0, -9464, 62, 56, 1.5);

    // Another guard
    SpawnCreature(1423, 0, -9450, 55, 56, 3.0);

    Print("2 guards spawned");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
