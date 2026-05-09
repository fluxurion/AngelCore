/*
 * Example 02: GameObjects + Bulk Spawn from Arrays
 * Spawn gameobjects and many creatures from a data table.
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    // ---- GameObjects ----
    SpawnGameObject(17900, 0, -9460, 60, 55, 0, 169, 0, 1);  // campfire
    SpawnGameObject(32348, 0, -9455, 50, 56, 2, 169, 0, 1);  // mailbox

    // ---- Bulk creatures from array ----
    // { entry, x, y, z, o, phaseId, respawn, level, faction, flags, gossip, equip, react }
    array<array<int>> spawns = {
        { 1423,  -9464,  62, 56, 150,  169, 300, 55, 11, 0x81, 0, 0, 1 },
        { 1423,  -9450,  55, 56, 300,  169, 300, 55, 11, 0x81, 0, 0, 1 },
        { 1423,  -9470,  70, 56, 50,   169, 300, 55, 11, 0x81, 0, 0, 1 },
        { 123456,-9460,  50, 56, 200,  169, 0,   60, 35, 0x83, 50001, 2, 0 },
        { 123457,-8960, -140, 84, 0,    169, 600, 63, 14, 0,    0,     1, 2 }
    };

    for (uint i = 0; i < spawns.length(); i++)
    {
        array<int>@ s = spawns[i];
        SpawnCreatureEx(s[0], 0,
            float(s[1])/100, float(s[2])/100, float(s[3])/100, float(s[4])/100,
            s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12]);
    }

    Print("Gameobjects + " + spawns.length() + " creatures spawned from array");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
