/*
 * Example 03: Creature & Spell Hooks
 * React to creature death, combat, and spell casts.
 */

#include "../includes/ScriptFramework.as"

// ---- Creature hooks ----

void OnCreatureDeath(Creature@ creature)
{
    if (creature is null) return;
    Print("[CreatureDeath] " + creature.GetName() + " (entry " + creature.GetEntry() + ") died");

    // Reset timers so stale events don't fire after death
    creature.ResetTimers();
}

void OnCreatureCombat(Creature@ creature)
{
    if (creature is null) return;
    Print("[Combat] " + creature.GetName() + " entered combat");
}

void OnCreatureLeaveCombat(Creature@ creature)
{
    if (creature is null) return;
    Print("[Combat] " + creature.GetName() + " left combat");
}

// ---- Spell hooks ----

void OnSpellCast(Spell@ spell)
{
    if (spell is null) return;

    Unit@ caster = spell.GetCaster();
    uint32 spellId = spell.GetSpellId();
    string casterName = caster !is null ? caster.GetName() : "unknown";

    Print("[SpellCast] " + casterName + " casts " + GetSpellName(spellId) + " (" + spellId + ")");
}

void main()
{
    RegisterCreatureScript(CREATURE_ON_DEATH,        @OnCreatureDeath);
    RegisterCreatureScript(CREATURE_ON_ENTER_COMBAT, @OnCreatureCombat);
    RegisterCreatureScript(CREATURE_ON_LEAVE_COMBAT, @OnCreatureLeaveCombat);
    RegisterSpellHook(SPELL_ON_CAST,                  @OnSpellCast);
    Print("[Hooks] Creature + Spell hooks registered");
}
