/*
 * Quest Encounter Example
 * Demonstrates:
 *   - Spawning creatures & gameobjects with phaseId
 *   - Per-creature configuration (level, faction, equipment, gossip, NPC flags)
 *   - Timed events (Schedule, Repeat, Cancel)
 *   - Creature scripting (Talk, Say, Emote, CastSpell, MovePoint)
 *   - Quest state checks and per-player phase changes
 *   - Cleanup on despawn
 *
 * This spawns an NPC that walks a patrol, talks at waypoints,
 * and casts spells when a player accepts a quest.
 */

#include "../includes/ScriptFramework.as"
#include "../includes/Common.as"

// ---- Event IDs ----
const uint32 EVENT_PATROL_MOVE = 1;
const uint32 EVENT_WAYPOINT_TALK = 2;
const uint32 EVENT_CHECK_PLAYERS = 3;
const uint32 EVENT_QUEST_REACTION = 4;
const uint32 EVENT_EMOTE_WAVE = 5;

// ---- Spawn data ----
const uint32 NPC_ENTRY = 123456;       // Your creature_template entry
const uint32 GO_ENTRY = 17900;         // GameObject entry (example: campfire)
const uint32 QUEST_ID = 60001;         // Quest that triggers this NPC's reaction
const uint32 PHASE_NORMAL = 169;       // Default phase
const uint32 PHASE_QUEST = 170;        // Phase during quest

// ---- Patrol waypoints ----
const float PATROL_POINTS[][3] = {
    { -8949.0f, -132.0f, 83.0f },
    { -8955.0f, -140.0f, 83.0f },
    { -8960.0f, -135.0f, 83.0f },
    { -8949.0f, -132.0f, 83.0f }  // Back to start (loop)
};
const uint32 PATROL_COUNT = 4;
int g_currentWaypoint = 0;

// ---- Spawn handles ----
uint64 g_npcGuid = 0;
uint64 g_goGuid = 0;

// ========================================================================
// Creature Update Hook — drives timed events for ALL AS-scripted creatures
// ========================================================================
void OnCreatureUpdate(Creature@ creature, uint32 diff)
{
    // Check if this creature has any scheduled events
    if (!creature.HasTimers())
        return;

    // Advance the timer and execute any due events
    uint32 eventId = creature.UpdateTimers(diff);
    while (eventId != 0)
    {
        // Route to the appropriate handler
        HandleTimedEvent(creature, eventId);

        // Get next event (if any were scheduled during HandleTimedEvent)
        eventId = creature.UpdateTimers(0);
    }
}

// ========================================================================
// Timed Event Handler
// ========================================================================
void HandleTimedEvent(Creature@ creature, uint32 eventId)
{
    switch (eventId)
    {
        case EVENT_PATROL_MOVE:
            OnPatrolMove(creature);
            break;

        case EVENT_WAYPOINT_TALK:
            OnWaypointTalk(creature);
            break;

        case EVENT_CHECK_PLAYERS:
            OnCheckPlayers(creature);
            break;

        case EVENT_QUEST_REACTION:
            OnQuestReaction(creature);
            break;

        case EVENT_EMOTE_WAVE:
            creature.DoEmote(1); // EMOTE_ONESHOT_WAVE
            creature.Say("Hello, traveler!");
            // Repeat every 10-30 seconds randomly
            creature.RepeatEventRandom(10000, 30000);
            break;
    }
}

// ========================================================================
// Event Handlers
// ========================================================================

void OnPatrolMove(Creature@ creature)
{
    if (creature is null) return;

    float x = PATROL_POINTS[g_currentWaypoint][0];
    float y = PATROL_POINTS[g_currentWaypoint][1];
    float z = PATROL_POINTS[g_currentWaypoint][2];

    creature.MovePoint(EVENT_PATROL_MOVE, x, y, z, true, 0.0, true);
    creature.Say("Moving to waypoint " + g_currentWaypoint + "...");

    g_currentWaypoint = (g_currentWaypoint + 1) % PATROL_COUNT;
    // Schedule next patrol move in 5 seconds (after arriving)
    creature.ScheduleEvent(EVENT_PATROL_MOVE, 5000);
}

void OnWaypointTalk(Creature@ creature)
{
    if (creature is null) return;
    creature.Yell("I have arrived at my post!");
    creature.DoEmote(22); // EMOTE_ONESHOT_CHEER
}

void OnCheckPlayers(Creature@ creature)
{
    if (creature is null) return;

    // Find nearby players who are on the quest
    // Note: In a real script, you'd iterate nearby players via the map
    // This is a simplified example
    Print("[Encounter] Checking for players on quest " + QUEST_ID);

    // Reschedule periodic check
    creature.ScheduleEvent(EVENT_CHECK_PLAYERS, 3000);
}

void OnQuestReaction(Creature@ creature)
{
    if (creature is null) return;
    creature.Yell("You dare approach me?! Die!");
    creature.DoEmote(15); // EMOTE_ONESHOT_ROAR

    // Cast a spell on self as a buff demonstration
    creature.CastSpellSelf(1238474); // Example spell

    // Move aggressively toward the center
    creature.MovePoint(99, -8950.0f, -135.0f, 83.0f, true, 5.0, false);
}

// ========================================================================
// Player Hook — when a player accepts the quest, phase them and trigger
// ========================================================================
void OnQuestAccept(Player@ player, uint32 questId)
{
    if (player is null || questId != QUEST_ID)
        return;

    Print("[Encounter] Player " + player.GetName() + " accepted quest " + questId);

    // Add the quest phase to the player
    PhasingHandler::AddPhase(@player, PHASE_QUEST, true);

    // Find our spawned NPC and trigger the reaction
    if (g_npcGuid != 0)
    {
        Creature@ npc = FindSpawnedCreature(g_npcGuid, player.GetMapId());
        if (npc !is null)
        {
            // Cancel the patrol and trigger combat reaction after 2 seconds
            npc.CancelEvent(EVENT_PATROL_MOVE);
            npc.ClearMovement();
            npc.ScheduleEvent(EVENT_QUEST_REACTION, 2000);
            npc.Say("Wait... I sense something...");
        }
    }
}

// ========================================================================
// Creature Death Hook — cleanup timers
// ========================================================================
void OnCreatureDeath(Creature@ creature)
{
    if (creature is null) return;
    // Reset timers on death (EventMap cleans itself)
    creature.ResetTimers();
    Print("[Encounter] NPC " + creature.GetName() + " died, timers reset");
}

// ========================================================================
// World Startup — spawn all entities
// ========================================================================
void SpawnEncounterEntities()
{
    Print("============================================");
    Print("[Encounter] Spawning quest encounter entities...");
    Print("============================================");

    // ----- Spawn the quest NPC -----
    g_npcGuid = SpawnCreature(
        NPC_ENTRY,           // entry
        0,                   // mapId (0 = Eastern Kingdoms)
        -8949.0f, -132.0f, 83.0f,  // x, y, z
        0.0f,                // orientation
        PHASE_NORMAL,        // phaseId
        300                  // respawn after 5 minutes
    );

    if (g_npcGuid != 0)
    {
        // Find the spawned creature to configure it
        Creature@ npc = FindSpawnedCreature(g_npcGuid, 0);
        if (npc !is null)
        {
            // Configure appearance & behavior
            npc.SetLevel(45);
            npc.SetFaction(14);                     // Hostile faction
            npc.SetNpcFlag(0x1);                    // UNIT_NPC_FLAG_GOSSIP
            npc.SetGossipMenu(50001);               // Custom gossip menu
            npc.SetEquipment(1);                    // Equipment set 1
            npc.SetReactState(REACT_DEFENSIVE);     // Defensive (won't aggro unless attacked)

            // Set display ID for a custom model (optional)
            // npc.SetDisplayId(12345);

            Print("[Encounter] NPC configured: level=" + npc.GetLevel() +
                  " faction=" + npc.GetFaction() +
                  " flags=" + npc.GetNpcFlag());

            // ----- Schedule timed events -----
            // Wave emote every 10-30 seconds (looping)
            npc.ScheduleEvent(EVENT_EMOTE_WAVE, 5000);

            // Start patrol after 3 seconds
            npc.ScheduleEvent(EVENT_PATROL_MOVE, 3000);

            // Say something when patrol reaches waypoint 1
            npc.ScheduleEvent(EVENT_WAYPOINT_TALK, 8000);

            // Periodic player check every 3 seconds
            npc.ScheduleEvent(EVENT_CHECK_PLAYERS, 5000, 0, 0);
        }
    }

    // ----- Spawn a gameobject near the NPC -----
    g_goGuid = SpawnGameObject(
        GO_ENTRY,            // entry
        0,                   // mapId
        -8950.0f, -135.0f, 83.0f,  // x, y, z
        0.0f,                // orientation
        PHASE_NORMAL,        // phaseId
        600,                 // respawn after 10 minutes
        1                    // GO_STATE_READY
    );

    if (g_goGuid != 0)
    {
        Print("[Encounter] GameObject spawned: " + g_goGuid);
    }

    Print("============================================");
    Print("[Encounter] Total spawned: " + GetCreatureSpawnCount() +
          " creatures, " + GetGameObjectSpawnCount() + " gameobjects");
    Print("============================================");
}

// ========================================================================
// Reload support — despawn current entities, then respawn fresh
// ========================================================================
void RespawnEncounter()
{
    Print("[Encounter] Reloading — despawning all entities...");
    DespawnAll();
    g_npcGuid = 0;
    g_goGuid = 0;
    g_currentWaypoint = 0;
    SpawnEncounterEntities();
}

// ========================================================================
// Main entry point
// ========================================================================
void main()
{
    Print("===================================================");
    Print("[Encounter] Quest Encounter Script loading...");
    Print("===================================================");

    // Register world startup hook to spawn entities
    RegisterWorldScript(WORLD_ON_STARTUP, @SpawnEncounterEntities);

    // Register creature update hook for timed events
    RegisterCreatureScript(CREATURE_ON_DEATH, @OnCreatureDeath);

    // Register player quest hook
    // Note: Use the quest hook registration from the quest API
    // RegisterQuestScript(QUEST_ON_ACCEPT, @OnQuestAccept);

    Print("[Encounter] Hooks registered: WORLD_ON_STARTUP, CREATURE_ON_DEATH");
    Print("[Encounter] Script loaded successfully!");
}

// ========================================================================
// Console command to reload this specific encounter
// ========================================================================
void OnConsoleCommand(string& command)
{
    if (command == "reload encounter")
    {
        RespawnEncounter();
        command = ""; // Consume the command
    }
}
