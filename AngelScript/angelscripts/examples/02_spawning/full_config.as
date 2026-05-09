/*
 * Example 02: Full Config Spawn
 * SpawnCreatureEx — one call sets level, faction, flags, gossip, equipment, react state.
 *
 * Signature: SpawnCreatureEx(entry, mapId, x, y, z, o,
 *             phaseId, respawnDelay, level, faction, npcFlags,
 *             gossipMenuId, equipmentId, reactState)
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    // Quest giver — friendly, gossips, gives quests
    SpawnCreatureEx(123456, 0, -9460, 50, 56, 2.0,
        169,    // phaseId
        0,      // never respawn
        60,     // level
        35,     // faction (friendly)
        0x1 | 0x2,  // GOSSIP + QUESTGIVER
        50001,      // gossip menu ID
        2,          // equipment set
        0);         // passive

    // Hostile boss — aggressive, respawns in 10min
    SpawnCreatureEx(123457, 0, -8960, -140, 84, 0.0,
        169,    // phaseId
        600,    // respawn after 600 seconds
        63,     // level
        14,     // faction (hostile)
        0,      // no gossip/quest
        0,      // no gossip menu
        1,      // equipment set 1
        2);     // aggressive

    Print("Quest NPC and boss spawned");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
