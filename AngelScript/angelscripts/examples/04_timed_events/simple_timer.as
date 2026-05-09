/*
 * Example 04: Simple Timer
 * An NPC that waves and speaks periodically.
 */

#include "../includes/ScriptFramework.as"

const uint32 EVENT_EMOTE = 1;

void HandleEvent(Creature@ creature, uint32 eventId)
{
    if (creature is null) return;

    if (eventId == EVENT_EMOTE)
    {
        creature.DoEmote(1);  // EMOTE_ONESHOT_WAVE
        creature.Say("Hello, traveler!");

        // Repeat every 10-30 seconds
        creature.RepeatEventRandom(10000, 30000);
    }
}

// Timer update — called for creatures with scheduled events
void OnCreatureUpdate(Creature@ creature, uint32 diff)
{
    if (!creature.HasTimers())
        return;

    uint32 eventId = creature.UpdateTimers(diff);
    if (eventId != 0)
        HandleEvent(creature, eventId);
}

void OnStartup()
{
    // Spawn the NPC
    uint64 guid = SpawnCreatureEx(123456, 0, -9460, 50, 56, 2.0,
        169, 0, 60, 35, 0x1, 50001, 2, 0);

    Creature@ npc = FindSpawnedCreature(guid, 0);
    if (npc !is null)
    {
        // First wave after 5 seconds
        npc.ScheduleEvent(EVENT_EMOTE, 5000);
    }

    Print("[TimerNPC] Spawned with looping emote timer");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
