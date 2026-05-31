#include "../../includes/ScriptFramework.as"

const uint32 SPELL_HARANIR_THORN_BLOOM_HEAL   = 1238467;
const uint32 SPELL_HARANIR_THORN_BLOOM_DAMAGE = 1238474;

// Direct Coefficients (AP-based)
const float HEAL_DIRECT_COEFF = 0.650f; // Eff 0
const float HEAL_PERIOD_COEFF = 0.150f; // Eff 1 (Per tick)
const float DMG_DIRECT_COEFF  = 0.500f; // Eff 0
const float DMG_PERIOD_COEFF  = 0.125f; // Eff 1 (Per tick = AP * 0.125 * vers, 12 ticks)

// ============================================================
// ON_CAST: Set base points directly (like C++ SetSpellValue)
// ============================================================
void OnThornBloomDamageCast(Spell@ spell, Unit@ target)
{
    if (spell is null) return;
    Unit@ caster = spell.GetCaster();
    if (caster is null) return;

    float ap = float(caster.GetTotalAttackPowerValue());
    float versPct = caster.GetVersatilityBonus();
    float versMul = 1.0f + (versPct / 100.0f);

    // Eff 0 = Direct Damage
    float bp0 = ap * DMG_DIRECT_COEFF * versMul;
    spell.SetBasePoint(0, bp0);

    // Eff 1 = Periodic Damage tick
    float bp1 = ap * DMG_PERIOD_COEFF * versMul;
    spell.SetBasePoint(1, bp1);
}

void OnThornBloomHealCast(Spell@ spell, Unit@ target)
{
    if (spell is null) return;
    Unit@ caster = spell.GetCaster();
    if (caster is null) return;

    float ap = float(caster.GetTotalAttackPowerValue());
    float versPct = caster.GetVersatilityBonus();
    float versMul = 1.0f + (versPct / 100.0f);

    // Eff 0 = Direct Heal
    float bp0 = ap * HEAL_DIRECT_COEFF * versMul;
    spell.SetBasePoint(0, bp0);

    // Eff 1 = Periodic Heal tick
    float bp1 = ap * HEAL_PERIOD_COEFF * versMul;
    spell.SetBasePoint(1, bp1);
}

// ============================================================
// AURA CALC AMOUNT HOOK - Override final calculated amount
// ============================================================
void OnThornBloomAuraCalcAmount(AuraEffect@ aurEff, double &out amount, bool &out canBeRecalculated)
{
    if (aurEff is null) return;
    Unit@ caster = aurEff.GetCaster();
    if (caster is null) return;

    float ap = float(caster.GetTotalAttackPowerValue());
    float versPct = caster.GetVersatilityBonus();
    float versMul = 1.0f + (versPct / 100.0f);

    float coeff = 0.0f;
    uint8 effIndex = aurEff.GetEffIndex();

    if (aurEff.GetId() == SPELL_HARANIR_THORN_BLOOM_HEAL)
    {
        if (effIndex == 0) coeff = HEAL_DIRECT_COEFF;
        else if (effIndex == 1) coeff = HEAL_PERIOD_COEFF;
    }
    else if (aurEff.GetId() == SPELL_HARANIR_THORN_BLOOM_DAMAGE)
    {
        if (effIndex == 0) coeff = DMG_DIRECT_COEFF;
        else if (effIndex == 1) coeff = DMG_PERIOD_COEFF;
    }

    if (coeff > 0.0f)
    {
        // Override the final amount - this replaces the core's calculated value entirely
        double finalAmount = double(ap * coeff * versMul);
        amount = finalAmount;
        aurEff.SetBaseAmount(finalAmount); // Update tooltip display
    }
}

void main()
{
    // Set base points on cast (like C++ SetSpellValue)
    RegisterSpellScript(SPELL_HARANIR_THORN_BLOOM_DAMAGE, SPELL_ON_CAST, @OnThornBloomDamageCast);
    RegisterSpellScript(SPELL_HARANIR_THORN_BLOOM_HEAL,   SPELL_ON_CAST, @OnThornBloomHealCast);

    // Override final calculated amount (prevents core from adding variance/level/mastery)
    RegisterSpellScript(SPELL_HARANIR_THORN_BLOOM_DAMAGE, SPELL_ON_AURA_CALC_AMOUNT, @OnThornBloomAuraCalcAmount);
    RegisterSpellScript(SPELL_HARANIR_THORN_BLOOM_HEAL,   SPELL_ON_AURA_CALC_AMOUNT, @OnThornBloomAuraCalcAmount);

}
