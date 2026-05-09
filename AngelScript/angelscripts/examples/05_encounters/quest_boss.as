/*
 * Example 05: Quest Boss Encounter
 * A boss that patrols, taunts nearby players, and casts spells in combat.
 * Uses: SpawnCreatureEx, ConfigureCreature, ScheduleEvent, CancelEvent,
 *       MovePoint, Say, Yell, CastSpell, DoEmote, RepeatEventRandom.
 */

#include "../includes/ScriptFramework.as"

const uint32 EVENT_PATROL     = 1;
const uint32 EVENT_TAUNT      = 2;
const uint32 EVENT_CAST_SPELL = 3;
const uint32 EVENT_CHECK      = 4;

const uint32 SPELL_FIREBALL   = 133;
const uint32 SPELL_FROSTBOLT  = 116;

const float ROUTE[][3] = {
    { -8960, -130, 84 },
    { -8945, -140, 83 },
    { -8930, -125, 82 },
    { -8960, -130, 84 }
};
int g_wpIdx = 0;

void HandleEvent(Creature@ boss, uint32 eventId)
{
    if (boss is null) return;

    switch (eventId)
    {
    case EVENT_PATROL:
        boss.MovePoint(1, ROUTE[g_wpIdx][0], ROUTE[g_wpIdx][1], ROUTE[g_wpIdx][2], true, 0, true);
        g_wpIdx = (g_wpIdx + 1) % 4;
        boss.ScheduleEvent(EVENT_PATROL, 7000);
        break;

    case EVENT_TAUNT:
        boss.Yell("Who dares disturb my slumber?!");
        boss.DoEmote(15); // ROAR
        boss.RepeatEventRandom(20000, 40000);
        break;

    case EVENT_CAST_SPELL:
        boss.Say("Take this!");
        boss.CastSpellSelf(SPELL_FROSTBOLT);
        boss.ScheduleEvent(EVENT_CAST_SPELL, 5000);
        break;

    case EVENT_CHECK:
        boss.Say("I smell intruders...");
        boss.ScheduleEvent(EVENT_CHECK, 10000);
        break;
    }
}

void OnCreatureCombat(Creature@ boss)
{
    if (boss is null) return;
    // Stop patrol, start fighting
    boss.CancelEvent(EVENT_PATROL);
    boss.CancelEvent(EVENT_CHECK);
    boss.ClearMovement();
    boss.Yell("You will regret this!");
    boss.ScheduleEvent(EVENT_CAST_SPELL, 1000);
}

void OnCreatureLeaveCombat(Creature@ boss)
{
    if (boss is null) return;
    boss.CancelEvent(EVENT_CAST_SPELL);
    boss.Say("Pathetic...");
    g_wpIdx = 0;
    boss.ScheduleEvent(EVENT_PATROL, 5000);
    boss.ScheduleEvent(EVENT_TAUNT, 15000);
    boss.ScheduleEvent(EVENT_CHECK, 10000);
}

void OnCreatureDeath(Creature@ boss)
{
    if (boss is null) return;
    boss.ResetTimers();
}

void OnStartup()
{
    uint64 guid = SpawnCreatureEx(123457, 0, -8960, -130, 84, 0,
        169, 600, 63, 14, 0, 0, 1, 2); // aggressive boss

    Creature@ boss = FindSpawnedCreature(guid, 0);
    if (boss !is null)
    {
        boss.ScheduleEvent(EVENT_PATROL, 3000);
        boss.ScheduleEvent(EVENT_TAUNT, 15000);
        boss.ScheduleEvent(EVENT_CHECK, 10000);
    }

    Print("[Boss] Spawned: patrols, taunts, fights, respawns in 10 min");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
    RegisterCreatureScript(CREATURE_ON_ENTER_COMBAT, @OnCreatureCombat);
    RegisterCreatureScript(CREATURE_ON_LEAVE_COMBAT, @OnCreatureLeaveCombat);
    RegisterCreatureScript(CREATURE_ON_DEATH, @OnCreatureDeath);
}
