/*
 * AngelScript Unit API
 * Wrapper functions and type registration for Unit (base of Player/Creature)
 */

#ifndef ANGELSCRIPT_INTEGRATION
    #error "ANGELSCRIPT_INTEGRATION macro must be defined"
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#pragma push_macro("IN")
#pragma push_macro("OUT")
#pragma push_macro("OPTIONAL")
#undef IN
#undef OUT
#undef OPTIONAL

#include <angelscript.h>

#pragma pop_macro("OPTIONAL")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")

#include "SpellAuras.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "Log.h"
#include "SpellAuraDefines.h"

namespace AngelScript
{
    // ---- Unit wrapper functions ----
    static std::string Unit_GetName(Unit* u) { return u ? u->GetName() : ""; }
    static uint8 Unit_GetLevel(Unit* u) { return u ? u->GetLevel() : 0; }
    static uint8 Unit_GetClass(Unit* u) { return u ? u->GetClass() : 0; }
    static uint8 Unit_GetRace(Unit* u) { return u ? u->GetRace() : 0; }
    static uint8 Unit_GetGender(Unit* u) { return u ? static_cast<uint8>(u->GetGender()) : 0; }
    static Unit* Unit_GetAuraCaster(uint32 spellId, Unit* u)
    {
        if (!u) return nullptr;
        Aura* aur = u->GetAura(spellId);
        return aur ? aur->GetCaster() : nullptr;
    }
    static uint32 Unit_GetGUIDLow(Unit* u) { return u ? static_cast<uint32>(u->GetGUID().GetCounter()) : 0; }
    static uint64 Unit_GetGUID(Unit* u) { return u ? u->GetGUID().GetRawValue(0) : 0; }
    static bool Unit_IsAlive(Unit* u) { return u ? u->IsAlive() : false; }
    static bool Unit_IsDead(Unit* u) { return u ? u->isDead() : false; }
    static bool Unit_IsInCombat(Unit* u) { return u ? u->IsInCombat() : false; }
    static bool Unit_IsMounted(Unit* u) { return u ? u->IsMounted() : false; }
    static uint64 Unit_GetHealth(Unit* u) { return u ? u->GetHealth() : 0; }
    static uint64 Unit_GetMaxHealth(Unit* u) { return u ? u->GetMaxHealth() : 0; }
    static void Unit_SetHealth(Unit* u, uint64 health) { if (u) u->SetHealth(health); }
    static void Unit_SetFullHealth(Unit* u) { if (u) u->SetFullHealth(); }
    static float Unit_GetHealthPct(Unit* u) { return u ? u->GetHealthPct() : 0.f; }
    static int32 Unit_GetPower(Unit* u, uint8 powerType) { return u ? u->GetPower(static_cast<Powers>(powerType)) : 0; }
    static int32 Unit_GetMaxPower(Unit* u, uint8 powerType) { return u ? u->GetMaxPower(static_cast<Powers>(powerType)) : 0; }
    static void Unit_SetPower(Unit* u, uint8 powerType, int32 value) { if (u) u->SetPower(static_cast<Powers>(powerType), value); }
    static bool Unit_HasAura(Unit* u, uint32 spellId) { return u ? u->HasAura(spellId) : false; }
    static uint32 Unit_GetAuraCount(Unit* u, uint32 spellId) { return u ? u->GetAuraCount(spellId) : 0; }
    static void Unit_RemoveAura(Unit* u, uint32 spellId) { if (u) u->RemoveAura(spellId); }
    static void Unit_RemoveAllAuras(Unit* u) { if (u) u->RemoveAllAuras(); }
    static uint32 Unit_GetFaction(Unit* u) { return u ? u->GetFaction() : 0; }
    static bool Unit_IsFriendlyTo(Unit* u, Unit* target) { return (u && target) ? u->IsFriendlyTo(target) : false; }
    static bool Unit_IsHostileTo(Unit* u, Unit* target) { return (u && target) ? u->IsHostileTo(target) : false; }
    static bool Unit_HasUnitFlag(Unit* u, uint32 flag) { return u ? u->HasUnitFlag(static_cast<UnitFlags>(flag)) : false; }
    static float Unit_GetDistanceTo(Unit* u, Unit* target) { return (u && target) ? u->GetDistance(target) : 0.f; }
    static bool Unit_IsWithinDist(Unit* u, Unit* target, float dist) { return (u && target) ? u->IsWithinDist(target, dist) : false; }
    static uint32 Unit_GetMapId(Unit* u) { return u ? u->GetMapId() : 0; }
    static float Unit_GetPositionX(Unit* u) { return u ? u->GetPositionX() : 0.f; }
    static float Unit_GetPositionY(Unit* u) { return u ? u->GetPositionY() : 0.f; }
    static float Unit_GetPositionZ(Unit* u) { return u ? u->GetPositionZ() : 0.f; }
    static float Unit_GetOrientation(Unit* u) { return u ? u->GetOrientation() : 0.f; }
    static void Unit_Kill(Unit* u, Unit* victim) { if (u && victim) Unit::Kill(u, victim, true); }

    // ---- NEW: Missing critical API wrappers ----
    static void Unit_CastSpell(Unit* u, Unit* target, uint32 spellId) { if (u && target) u->CastSpell(target, spellId, true); }
    static void Unit_CastSpellSelf(Unit* u, uint32 spellId) { if (u) u->CastSpell(u, spellId, true); }
    static void Unit_AddAura(Unit* u, uint32 spellId, Unit* caster) { if (u && caster) u->AddAura(spellId, caster); }
    static void Unit_Attack(Unit* u, Unit* target, bool melee) { if (u && target) u->Attack(target, melee); }
    static void Unit_AttackStop(Unit* u) { if (u) u->AttackStop(); }
    static bool Unit_IsInWorld(Unit* u) { return u ? u->IsInWorld() : false; }
    static void Unit_SetUnitFlag(Unit* u, uint32 flag) { if (u) u->SetUnitFlag(static_cast<UnitFlags>(flag)); }
    static void Unit_RemoveUnitFlag(Unit* u, uint32 flag) { if (u) u->RemoveUnitFlag(static_cast<UnitFlags>(flag)); }
    static bool Unit_IsPlayer(Unit* u) { return u ? u->IsPlayer() : false; }
    static bool Unit_IsCreature(Unit* u) { return u ? u->IsCreature() : false; }

    static float Unit_GetTotalAttackPowerValue(Unit* u) { return u ? u->GetTotalAttackPowerValue(BASE_ATTACK) : 0.f; }
    static float Unit_GetVersatilityBonus(Unit* u)
    {
        if (!u) return 0.f;
        if (Player* p = u->ToPlayer())
            return p->GetRatingBonusValue(CR_VERSATILITY_DAMAGE_DONE) + float(p->GetTotalAuraModifier(SPELL_AURA_MOD_VERSATILITY));
        return 0.f;
    }
    
    // ---- AuraEffect wrapper functions ----
    static uint32 AuraEffect_GetId(AuraEffect* ae) { return ae ? ae->GetId() : 0; }
    static uint8 AuraEffect_GetEffIndex(AuraEffect* ae) { return ae ? ae->GetEffIndex() : 0; }
    static double AuraEffect_GetAmount(AuraEffect* ae) { return ae ? ae->GetAmount() : 0; }
    static void AuraEffect_SetAmount(AuraEffect* ae, double amount) { if (ae) ae->SetAmount(amount); }
    static Unit* AuraEffect_GetCaster(AuraEffect* ae) { return ae ? ae->GetCaster() : nullptr; }
    static Unit* AuraEffect_GetOwner(AuraEffect* ae) { return (ae && ae->GetBase() && ae->GetBase()->GetOwner()) ? ae->GetBase()->GetOwner()->ToUnit() : nullptr; }


    // Cast methods — critical for type dispatch
    static Player* Unit_ToPlayer(Unit* u) { return u ? u->ToPlayer() : nullptr; }
    static Creature* Unit_ToCreature(Unit* u) { return u ? u->ToCreature() : nullptr; }

    void RegisterUnitAPI(asIScriptEngine* _scriptEngine)
    {
        int r = _scriptEngine->RegisterObjectType("Unit", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register Unit type: {}", r);
            return;
        }

        // Basic info
        r = _scriptEngine->RegisterObjectMethod("Unit", "string GetName() const", asFUNCTION(Unit_GetName), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint8 GetLevel() const", asFUNCTION(Unit_GetLevel), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint8 GetClass() const", asFUNCTION(Unit_GetClass), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint8 GetRace() const", asFUNCTION(Unit_GetRace), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint8 GetGender() const", asFUNCTION(Unit_GetGender), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint64 GetGUID() const", asFUNCTION(Unit_GetGUID), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint32 GetGUIDLow() const", asFUNCTION(Unit_GetGUIDLow), asCALL_CDECL_OBJFIRST);

        // Status
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsAlive() const", asFUNCTION(Unit_IsAlive), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsDead() const", asFUNCTION(Unit_IsDead), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsInCombat() const", asFUNCTION(Unit_IsInCombat), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsMounted() const", asFUNCTION(Unit_IsMounted), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsInWorld() const", asFUNCTION(Unit_IsInWorld), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsPlayer() const", asFUNCTION(Unit_IsPlayer), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsCreature() const", asFUNCTION(Unit_IsCreature), asCALL_CDECL_OBJFIRST);

        // Cast methods — critical
        r = _scriptEngine->RegisterObjectMethod("Unit", "Unit@ GetAuraCaster(uint32 spellId)", asFUNCTION(Unit_GetAuraCaster), asCALL_CDECL_OBJLAST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "Player@ ToPlayer() const", asFUNCTION(Unit_ToPlayer), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "Creature@ ToCreature() const", asFUNCTION(Unit_ToCreature), asCALL_CDECL_OBJFIRST);

        // Health
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint64 GetHealth() const", asFUNCTION(Unit_GetHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint64 GetMaxHealth() const", asFUNCTION(Unit_GetMaxHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void SetHealth(uint64)", asFUNCTION(Unit_SetHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void SetFullHealth()", asFUNCTION(Unit_SetFullHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetHealthPct() const", asFUNCTION(Unit_GetHealthPct), asCALL_CDECL_OBJFIRST);

        // Power
        r = _scriptEngine->RegisterObjectMethod("Unit", "int32 GetPower(uint8) const", asFUNCTION(Unit_GetPower), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "int32 GetMaxPower(uint8) const", asFUNCTION(Unit_GetMaxPower), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void SetPower(uint8, int32)", asFUNCTION(Unit_SetPower), asCALL_CDECL_OBJFIRST);

        // Auras
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool HasAura(uint32) const", asFUNCTION(Unit_HasAura), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint32 GetAuraCount(uint32) const", asFUNCTION(Unit_GetAuraCount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void RemoveAura(uint32)", asFUNCTION(Unit_RemoveAura), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void RemoveAllAuras()", asFUNCTION(Unit_RemoveAllAuras), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void AddAura(uint32, Unit@)", asFUNCTION(Unit_AddAura), asCALL_CDECL_OBJFIRST);

        // Spells
        r = _scriptEngine->RegisterObjectMethod("Unit", "void CastSpell(Unit@, uint32)", asFUNCTION(Unit_CastSpell), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void CastSpellSelf(uint32)", asFUNCTION(Unit_CastSpellSelf), asCALL_CDECL_OBJFIRST);

        // Combat
        r = _scriptEngine->RegisterObjectMethod("Unit", "void Attack(Unit@, bool)", asFUNCTION(Unit_Attack), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void AttackStop()", asFUNCTION(Unit_AttackStop), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void Kill(Unit@)", asFUNCTION(Unit_Kill), asCALL_CDECL_OBJFIRST);

        // Faction
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint32 GetFaction() const", asFUNCTION(Unit_GetFaction), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsFriendlyTo(Unit@) const", asFUNCTION(Unit_IsFriendlyTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsHostileTo(Unit@) const", asFUNCTION(Unit_IsHostileTo), asCALL_CDECL_OBJFIRST);

        // Flags
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool HasUnitFlag(uint32) const", asFUNCTION(Unit_HasUnitFlag), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void SetUnitFlag(uint32)", asFUNCTION(Unit_SetUnitFlag), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "void RemoveUnitFlag(uint32)", asFUNCTION(Unit_RemoveUnitFlag), asCALL_CDECL_OBJFIRST);

        // Stats
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetTotalAttackPowerValue() const", asFUNCTION(Unit_GetTotalAttackPowerValue), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetVersatilityBonus() const",       asFUNCTION(Unit_GetVersatilityBonus),         asCALL_CDECL_OBJFIRST);

        // Distance
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetDistanceTo(Unit@) const", asFUNCTION(Unit_GetDistanceTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "bool IsWithinDist(Unit@, float) const", asFUNCTION(Unit_IsWithinDist), asCALL_CDECL_OBJFIRST);

        // Position
        r = _scriptEngine->RegisterObjectMethod("Unit", "uint32 GetMapId() const", asFUNCTION(Unit_GetMapId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetPositionX() const", asFUNCTION(Unit_GetPositionX), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetPositionY() const", asFUNCTION(Unit_GetPositionY), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetPositionZ() const", asFUNCTION(Unit_GetPositionZ), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Unit", "float GetOrientation() const", asFUNCTION(Unit_GetOrientation), asCALL_CDECL_OBJFIRST);

        TC_LOG_INFO("server.angelscript", "Unit API registered ({} methods)", 50);
    }

    void RegisterAuraAPI(asIScriptEngine* _scriptEngine)
    {
        int r = _scriptEngine->RegisterObjectType("AuraEffect", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register AuraEffect type: {}", r);
            return;
        }

        r = _scriptEngine->RegisterObjectMethod("AuraEffect", "uint32 GetId() const", asFUNCTION(AuraEffect_GetId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("AuraEffect", "uint8 GetEffIndex() const", asFUNCTION(AuraEffect_GetEffIndex), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("AuraEffect", "double GetAmount() const", asFUNCTION(AuraEffect_GetAmount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("AuraEffect", "void SetAmount(double)", asFUNCTION(AuraEffect_SetAmount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("AuraEffect", "Unit@ GetCaster() const", asFUNCTION(AuraEffect_GetCaster), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("AuraEffect", "Unit@ GetOwner() const", asFUNCTION(AuraEffect_GetOwner), asCALL_CDECL_OBJFIRST);

        TC_LOG_INFO("server.angelscript", "Aura API registered");
    }

} // namespace AngelScript
