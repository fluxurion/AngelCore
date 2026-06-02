/*
 * AngelScript Spell API
 * Wrapper functions and type registration for Spell
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

#include "Spell.h"
#include "SpellDefines.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "Log.h"

namespace AngelScript
{
    // ---- CastSpellExtraArgs wrapper ----
    static CastSpellExtraArgs* CastSpellExtraArgs_Factory()
    {
        return new CastSpellExtraArgs();
    }

    static CastSpellExtraArgs* CastSpellExtraArgs_FactoryTriggered(bool triggered)
    {
        return new CastSpellExtraArgs(triggered);
    }

    static void CastSpellExtraArgs_AddSpellMod(CastSpellExtraArgs* args, SpellValueMod mod, int32 val)
    {
        if (args) args->AddSpellMod(mod, val);
    }

    static void CastSpellExtraArgs_AddSpellModFloat(CastSpellExtraArgs* args, SpellValueModFloat mod, float val)
    {
        if (args) args->AddSpellMod(mod, val);
    }

    static void CastSpellExtraArgs_SetTriggerFlags(CastSpellExtraArgs* args, uint32 flags)
    {
        if (args) args->SetTriggerFlags(static_cast<TriggerCastFlags>(flags));
    }

    // ---- CastSpellTargetArg wrapper ----
    static CastSpellTargetArg* CastSpellTargetArg_Factory()
    {
        return new CastSpellTargetArg();
    }

    static CastSpellTargetArg* CastSpellTargetArg_FactoryUnit(Unit* target)
    {
        return new CastSpellTargetArg(target);
    }

    // ---- Spell wrapper functions ----
    static uint32 Spell_GetSpellId(Spell* s) { return s ? s->GetSpellInfo()->Id : 0; }
    static Unit* Spell_GetCaster(Spell* s) { if (!s) return nullptr; WorldObject* caster = s->GetCaster(); return caster ? caster->ToUnit() : nullptr; }
    static Unit* Spell_GetOriginalCaster(Spell* s) { if (!s) return nullptr; Unit* oc = s->GetOriginalCaster(); return oc; }
    static int32 Spell_GetCastTime(Spell* s) { return s ? s->GetCastTime() : 0; }
    static void Spell_Cancel(Spell* s) { if (s) s->cancel(); }
    static void Spell_Finish(Spell* s) { if (s) s->finish(); }
    static bool Spell_IsTriggered(Spell* s) { return s ? s->IsTriggered() : false; }
    static bool Spell_IsChannelActive(Spell* s) { return s ? s->IsChannelActive() : false; }

    // ---- NEW: Missing critical API wrappers ----
    static Unit* Spell_GetTarget(Spell* s) { return s ? s->m_targets.GetUnitTarget() : nullptr; }
    static uint8 Spell_GetEffectIndex(Spell* /*s*/) { return 0; } // Context-dependent, placeholder
    static bool Spell_IsAutoRepeat(Spell* s) { return s ? s->IsAutoRepeat() : false; }
    static bool Spell_IsNextMeleeSwingSpell(Spell* s) { return s ? s->GetSpellInfo()->HasAttribute(SPELL_ATTR0_ON_NEXT_SWING) : false; }
    static bool Spell_IsRangedSpell(Spell* s) { return s ? s->GetSpellInfo()->IsRangedWeaponSpell() : false; }
    static bool Spell_IsMeleeAttack(Spell* s) { return s ? s->GetSpellInfo()->HasAttribute(SPELL_ATTR0_ON_NEXT_SWING) : false; }
    static bool Spell_IsAttackSpell(Spell* s) { return s ? s->GetSpellInfo()->IsPositive() == false : false; }
    static bool Spell_IsPositiveSpell(Spell* s) { return s ? s->IsPositive() : false; }
    static bool Spell_IsChanneledSpell(Spell* s) { return s ? s->GetSpellInfo()->IsChanneled() : false; }
    static float Spell_GetRange(Spell* s) { return s ? s->GetSpellInfo()->GetMaxRange() : 0.f; }
    static uint32 Spell_GetPowerCost(Spell* s) { 
        if (!s) return 0; 
        auto costs = s->GetPowerCost();
        uint32 total = 0;
        for (const auto& cost : costs)
            total += cost.Amount;
        return total;
    }
    static uint8 Spell_GetSpellSchool(Spell* s) { return s ? static_cast<uint8>(s->GetSpellInfo()->SchoolMask) : 0; }

    // Hit damage / healing setters (via public Spell accessors)
    static int32  Spell_GetHitDamage(Spell* s)            { return s ? s->GetHitDamage() : 0; }
    static void   Spell_SetHitDamage(Spell* s, int32 val) { if (s) s->SetHitDamage(val); }
    static int32  Spell_GetHitHeal(Spell* s)              { return s ? s->GetHitHeal()   : 0; }
    static void   Spell_SetHitHeal(Spell* s, int32 val)   { if (s) s->SetHitHeal(val); }

    // Set base point for a specific effect index (0-31)
    static void   Spell_SetBasePoint(Spell* s, uint8 effIndex, float val)
    {
        if (!s) return;
        s->SetSpellBasePoint(effIndex, val);
    }
    static float  Spell_GetBasePoint(Spell* s, uint8 effIndex)
    {
        if (!s) return 0.0f;
        return s->GetSpellBasePoint(effIndex);
    }

    void RegisterSpellAPI(asIScriptEngine* _scriptEngine)
    {
        int r;

        // Register CastSpellExtraArgs type
        r = _scriptEngine->RegisterObjectType("CastSpellExtraArgs", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register CastSpellExtraArgs type: {}", r);
        }
        else
        {
            r = _scriptEngine->RegisterObjectBehaviour("CastSpellExtraArgs", asBEHAVE_FACTORY, "CastSpellExtraArgs@ f()", asFUNCTION(CastSpellExtraArgs_Factory), asCALL_CDECL);
            r = _scriptEngine->RegisterObjectBehaviour("CastSpellExtraArgs", asBEHAVE_FACTORY, "CastSpellExtraArgs@ f(bool)", asFUNCTION(CastSpellExtraArgs_FactoryTriggered), asCALL_CDECL);
            r = _scriptEngine->RegisterObjectMethod("CastSpellExtraArgs", "void AddSpellMod(int32, int32)", asFUNCTION(CastSpellExtraArgs_AddSpellMod), asCALL_CDECL_OBJFIRST);
            r = _scriptEngine->RegisterObjectMethod("CastSpellExtraArgs", "void AddSpellModFloat(int32, float)", asFUNCTION(CastSpellExtraArgs_AddSpellModFloat), asCALL_CDECL_OBJFIRST);
            r = _scriptEngine->RegisterObjectMethod("CastSpellExtraArgs", "void SetTriggerFlags(uint32)", asFUNCTION(CastSpellExtraArgs_SetTriggerFlags), asCALL_CDECL_OBJFIRST);
        }

        // Register CastSpellTargetArg type
        r = _scriptEngine->RegisterObjectType("CastSpellTargetArg", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register CastSpellTargetArg type: {}", r);
        }
        else
        {
            r = _scriptEngine->RegisterObjectBehaviour("CastSpellTargetArg", asBEHAVE_FACTORY, "CastSpellTargetArg@ f()", asFUNCTION(CastSpellTargetArg_Factory), asCALL_CDECL);
            r = _scriptEngine->RegisterObjectBehaviour("CastSpellTargetArg", asBEHAVE_FACTORY, "CastSpellTargetArg@ f(Unit@)", asFUNCTION(CastSpellTargetArg_FactoryUnit), asCALL_CDECL);
        }

        // Register Spell type
        r = _scriptEngine->RegisterObjectType("Spell", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register Spell type: {}", r);
            return;
        }

        // Basic info
        r = _scriptEngine->RegisterObjectMethod("Spell", "uint32 GetSpellId() const", asFUNCTION(Spell_GetSpellId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "Unit@ GetCaster() const", asFUNCTION(Spell_GetCaster), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "Unit@ GetOriginalCaster() const", asFUNCTION(Spell_GetOriginalCaster), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "int32 GetCastTime() const", asFUNCTION(Spell_GetCastTime), asCALL_CDECL_OBJFIRST);

        // Control
        r = _scriptEngine->RegisterObjectMethod("Spell", "void Cancel()", asFUNCTION(Spell_Cancel), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "void Finish()", asFUNCTION(Spell_Finish), asCALL_CDECL_OBJFIRST);

        // Status
        r = _scriptEngine->RegisterObjectMethod("Spell", "bool IsTriggered() const", asFUNCTION(Spell_IsTriggered), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "bool IsChannelActive() const", asFUNCTION(Spell_IsChannelActive), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "bool IsAutoRepeat() const", asFUNCTION(Spell_IsAutoRepeat), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "bool IsChanneledSpell() const", asFUNCTION(Spell_IsChanneledSpell), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "bool IsPositiveSpell() const", asFUNCTION(Spell_IsPositiveSpell), asCALL_CDECL_OBJFIRST);

        // Target
        r = _scriptEngine->RegisterObjectMethod("Spell", "Unit@ GetTarget() const", asFUNCTION(Spell_GetTarget), asCALL_CDECL_OBJFIRST);

        // Hit damage / healing override
        r = _scriptEngine->RegisterObjectMethod("Spell", "int32 GetHitDamage() const",    asFUNCTION(Spell_GetHitDamage), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "void SetHitDamage(int32)",       asFUNCTION(Spell_SetHitDamage), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "int32 GetHitHeal() const",       asFUNCTION(Spell_GetHitHeal),   asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "void SetHitHeal(int32)",         asFUNCTION(Spell_SetHitHeal),   asCALL_CDECL_OBJFIRST);

        // Base point override (effect index 0-31)
        r = _scriptEngine->RegisterObjectMethod("Spell", "void SetBasePoint(uint8, float)", asFUNCTION(Spell_SetBasePoint), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "float GetBasePoint(uint8) const", asFUNCTION(Spell_GetBasePoint), asCALL_CDECL_OBJFIRST);

        // Spell info
        r = _scriptEngine->RegisterObjectMethod("Spell", "float GetRange() const", asFUNCTION(Spell_GetRange), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "uint32 GetPowerCost() const", asFUNCTION(Spell_GetPowerCost), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Spell", "uint8 GetSpellSchool() const", asFUNCTION(Spell_GetSpellSchool), asCALL_CDECL_OBJFIRST);

        TC_LOG_INFO("server.angelscript", "Spell API registered ({} methods)", 20);
    }

} // namespace AngelScript
