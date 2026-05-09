#include "../includes/ScriptFramework.as"

// ============================================================
// Spawn a few NPCs in Goldshire
// ============================================================
void SpawnMyNPCs()
{
    // A guard
    SpawnCreature(1423, 0, -9464, 62, 56, 1.5);

    // A quest giver with gossip menu
    SpawnCreatureEx(123456, 0, -9460, 50, 56, 2.0, 169, 0,
        60, 35, 0x1 | 0x2, 50001, 2, 0);

    // A hostile enemy with equipment
    SpawnCreatureEx(123457, 0, -8960, -140, 84, 0.0, 169, 600,
        63, 14, 0, 0, 1, 2);
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @SpawnMyNPCs);
}
