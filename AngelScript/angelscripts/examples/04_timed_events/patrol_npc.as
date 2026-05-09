/*
 * Example 04: Patrol NPC
 * An NPC walking a patrol route using timed MovePoint calls.
 */

#include "../includes/ScriptFramework.as"

const uint32 EVENT_PATROL = 1;
const uint32 EVENT_TALK   = 2;

int g_waypoint = 0;

// Patrol waypoints: {x, y, z}
const float POINTS[][3] = {
    { -8949, -132, 83 },
    { -8955, -140, 83 },
    { -8960, -135, 83 },
    { -8949, -132, 83 }  // loop back to start
};

void HandleEvent(Creature@ npc, uint32 eventId)
{
    if (npc is null) return;

    if (eventId == EVENT_PATROL)
    {
        float x = POINTS[g_waypoint][0];
        float y = POINTS[g_waypoint][1];
        float z = POINTS[g_waypoint][2];

        npc.MovePoint(1, x, y, z, true, 0, true);
        g_waypoint = (g_waypoint + 1) % 4;

        // Next move in 6 seconds
        npc.ScheduleEvent(EVENT_PATROL, 6000);
    }
    else if (eventId == EVENT_TALK)
    {
        npc.Say("Patrolling waypoint " + g_waypoint + "...");
        npc.ScheduleEvent(EVENT_TALK, 15000); // Talk every 15 seconds
    }
}

void OnStartup()
{
    uint64 guid = SpawnCreature(123458, 0, -8949, -132, 83, 0, 169, 300);

    Creature@ npc = FindSpawnedCreature(guid, 0);
    if (npc !is null)
    {
        ConfigureCreature(npc, 50, 11, 0x81, 0, 1, 0); // passive guard
        npc.ScheduleEvent(EVENT_PATROL, 3000);
        npc.ScheduleEvent(EVENT_TALK, 10000);
    }

    Print("[Patrol] NPC spawned, patrol + talk timers started");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
