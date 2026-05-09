/*
 * Example 05: Multi-Phase Boss
 * A boss with 3 phases at 100%/60%/30% HP.
 * Phase changes trigger different spell patterns.
 * Uses: EventMap phases, CancelEventGroup, RescheduleEvent.
 */

#include "../includes/ScriptFramework.as"

// Event groups per phase
const uint32 GROUP_PHASE1 = 1;
const uint32 GROUP_PHASE2 = 2;
const uint32 GROUP_PHASE3 = 3;

// Events
const uint32 EVENT_FIREBALL  = 1;
const uint32 EVENT_FROSTBOLT = 2;
const uint32 EVENT_HEAL      = 3;
const uint32 EVENT_ENRAGE    = 4;
const uint32 EVENT_HP_CHECK  = 5;

void SchedulePhase1(Creature@ boss)
{
    boss.CancelEventGroup(GROUP_PHASE2);
    boss.CancelEventGroup(GROUP_PHASE3);
    boss.RemoveEventPhase(2);
    boss.RemoveEventPhase(3);
    boss.AddEventPhase(1);

    boss.Yell("Phase 1: Face my frost!");
    boss.ScheduleEvent(EVENT_FROSTBOLT, 1000, GROUP_PHASE1);
    boss.ScheduleEvent(EVENT_FIREBALL,  5000, GROUP_PHASE1);
}

void SchedulePhase2(Creature@ boss)
{
    boss.CancelEventGroup(GROUP_PHASE1);
    boss.RemoveEventPhase(1);
    boss.AddEventPhase(2);

    boss.Yell("Phase 2: I will heal!");
    boss.ScheduleEvent(EVENT_FROSTBOLT, 500,  GROUP_PHASE2);  // faster
    boss.ScheduleEvent(EVENT_HEAL,      8000, GROUP_PHASE2);  // self-heal
}

void SchedulePhase3(Creature@ boss)
{
    boss.CancelEventGroup(GROUP_PHASE2);
    boss.RemoveEventPhase(2);
    boss.AddEventPhase(3);

    boss.Yell("Phase 3: ENRAGE!");
    boss.DoEmote(15); // ROAR
    boss.ScheduleEvent(EVENT_ENRAGE, 500, GROUP_PHASE3);  // enrage aura
    boss.ScheduleEvent(EVENT_FIREBALL, 2000, GROUP_PHASE3);
}

void HandleEvent(Creature@ boss, uint32 eventId)
{
    if (boss is null) return;

    switch (eventId)
    {
    case EVENT_FIREBALL:
        boss.Say("Fire!");
        boss.CastSpellSelf(133);
        boss.RepeatEventRandom(3000, 6000);
        break;

    case EVENT_FROSTBOLT:
        boss.CastSpellSelf(116);
        boss.RepeatEventRandom(2000, 4000);
        break;

    case EVENT_HEAL:
        boss.Say("Healing...");
        boss.CastSpellSelf(2050); // Heal
        boss.RepeatEvent(12000);
        break;

    case EVENT_ENRAGE:
        boss.CastSpellSelf(26662); // Enrage
        boss.RepeatEvent(30000);   // refresh every 30s
        break;

    case EVENT_HP_CHECK:
        CheckPhaseChange(boss);
        boss.ScheduleEvent(EVENT_HP_CHECK, 1000); // check every 1s
        break;
    }
}

void CheckPhaseChange(Creature@ boss)
{
    float pct = boss.GetHealthPct();

    if (pct <= 30 && boss.IsInCombat())
        SchedulePhase3(boss);
    else if (pct <= 60 && boss.IsInCombat())
        SchedulePhase2(boss);
}

void OnCreatureCombat(Creature@ boss)
{
    if (boss is null) return;
    SchedulePhase1(boss);
    boss.ScheduleEvent(EVENT_HP_CHECK, 1000); // health checker
}

void OnCreatureDeath(Creature@ boss)
{
    if (boss is null) return;
    boss.Yell("Impossible...");
    boss.ResetTimers();
}

void OnStartup()
{
    uint64 guid = SpawnCreatureEx(123457, 0, -8960, -130, 84, 0,
        169, 600, 63, 14, 0, 0, 1, 2);

    Creature@ boss = FindSpawnedCreature(guid, 0);
    if (boss !is null)
        boss.Say("I await a worthy opponent...");

    Print("[MultiPhase] Boss spawned: 3 phases at 100/60/30% HP");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
    RegisterCreatureScript(CREATURE_ON_ENTER_COMBAT, @OnCreatureCombat);
    RegisterCreatureScript(CREATURE_ON_DEATH, @OnCreatureDeath);
}
