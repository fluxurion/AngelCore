/*
 * AngelScript Creature API
 * Wrapper functions and type registration for Creature
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

#include "Creature.h"
#include "Unit.h"
#include "Player.h"
#include "CreatureAI.h"
#include "Log.h"

namespace AngelScript
{
    // ---- Creature wrapper functions ----
    static std::string Creature_GetName(Creature* c) { return c ? c->GetName() : ""; }
    static uint32 Creature_GetEntry(Creature* c) { return c ? c->GetEntry() : 0; }
    static uint64 Creature_GetGUID(Creature* c) { return c ? c->GetGUID().GetRawValue(0) : 0; }
    static uint32 Creature_GetGUIDLow(Creature* c) { return c ? static_cast<uint32>(c->GetGUID().GetCounter()) : 0; }
    static uint32 Creature_GetSpawnId(Creature* c) { return c ? c->GetSpawnId() : 0; }
    static uint8 Creature_GetLevel(Creature* c) { return c ? c->GetLevel() : 0; }
    static bool Creature_IsAlive(Creature* c) { return c ? c->IsAlive() : false; }
    static bool Creature_IsDead(Creature* c) { return c ? c->isDead() : false; }
    static bool Creature_IsInCombat(Creature* c) { return c ? c->IsInCombat() : false; }
    static bool Creature_IsInCombatWith(Creature* c, Unit* target) { return (c && target) ? c->IsInCombatWith(target) : false; }
    static bool Creature_CanHaveThreatList(Creature* c) { return c ? c->CanHaveThreatList() : false; }
    static void Creature_SetInCombatWith(Creature* c, Unit* target) { if (c && target) c->SetInCombatWith(target); }
    static uint64 Creature_GetHealth(Creature* c) { return c ? c->GetHealth() : 0; }
    static uint64 Creature_GetMaxHealth(Creature* c) { return c ? c->GetMaxHealth() : 0; }
    static void Creature_SetHealth(Creature* c, uint64 health) { if (c) c->SetHealth(health); }
    static void Creature_SetFullHealth(Creature* c) { if (c) c->SetFullHealth(); }
    static float Creature_GetHealthPct(Creature* c) { return c ? c->GetHealthPct() : 0.f; }
    static bool Creature_HasAura(Creature* c, uint32 spellId) { return c ? c->HasAura(spellId) : false; }
    static uint8 Creature_GetReactState(Creature* c) { return c ? static_cast<uint8>(c->GetReactState()) : 0; }
    static void Creature_SetReactState(Creature* c, uint8 state) { if (c) c->SetReactState(static_cast<ReactStates>(state)); }
    static bool Creature_HasLootRecipient(Creature* c) { return c ? c->hasLootRecipient() : false; }
    static bool Creature_IsTappedBy(Creature* c, Player* p) { return (c && p) ? c->isTappedBy(p) : false; }
    static bool Creature_HasQuest(Creature* c, uint32 questId) { return c ? c->hasQuest(questId) : false; }
    static void Creature_Respawn(Creature* c, bool force) { if (c) c->Respawn(force); }
    static void Creature_DespawnOrUnsummon(Creature* c, uint32 msTime) { if (c) c->DespawnOrUnsummon(Milliseconds(msTime)); }
    static uint32 Creature_GetFaction(Creature* c) { return c ? c->GetFaction() : 0; }
    static bool Creature_IsFriendlyTo(Creature* c, Unit* target) { return (c && target) ? c->IsFriendlyTo(target) : false; }
    static bool Creature_IsHostileTo(Creature* c, Unit* target) { return (c && target) ? c->IsHostileTo(target) : false; }
    static uint32 Creature_GetMapId(Creature* c) { return c ? c->GetMapId() : 0; }
    static float Creature_GetPositionX(Creature* c) { return c ? c->GetPositionX() : 0.f; }
    static float Creature_GetPositionY(Creature* c) { return c ? c->GetPositionY() : 0.f; }
    static float Creature_GetPositionZ(Creature* c) { return c ? c->GetPositionZ() : 0.f; }
    static float Creature_GetOrientation(Creature* c) { return c ? c->GetOrientation() : 0.f; }

    // ---- NEW: Missing critical API wrappers ----
    static void Creature_CastSpell(Creature* c, Unit* target, uint32 spellId) { if (c && target) c->CastSpell(target, spellId, true); }
    static void Creature_CastSpell(Creature* c, Unit* target, uint32 spellId, bool triggered) { if (c && target) c->CastSpell(target, spellId, triggered); }
    static void Creature_CastSpell(Creature* c, Unit* target, uint32 spellId, CastSpellExtraArgs const& args) { if (c && target) c->CastSpell(target, spellId, args); }
    static void Creature_CastSpell(Creature* c, CastSpellTargetArg const& targetArg, uint32 spellId, CastSpellExtraArgs const& args) { if (c) c->CastSpell(targetArg, spellId, args); }
    static void Creature_CastSpellSelf(Creature* c, uint32 spellId) { if (c) c->CastSpell(c, spellId, true); }
    static void Creature_CastSpellSelf(Creature* c, uint32 spellId, bool triggered) { if (c) c->CastSpell(c, spellId, triggered); }
    static void Creature_AddAura(Creature* c, uint32 spellId, Unit* caster) { if (c && caster) c->AddAura(spellId, caster); }
    static void Creature_RemoveAura(Creature* c, uint32 spellId) { if (c) c->RemoveAura(spellId); }
    static void Creature_RemoveAllAuras(Creature* c) { if (c) c->RemoveAllAuras(); }
    static uint32 Creature_GetAuraCount(Creature* c, uint32 spellId) { return c ? c->GetAuraCount(spellId) : 0; }
    static int32 Creature_GetPower(Creature* c, uint8 powerType) { return c ? c->GetPower(static_cast<Powers>(powerType)) : 0; }
    static int32 Creature_GetMaxPower(Creature* c, uint8 powerType) { return c ? c->GetMaxPower(static_cast<Powers>(powerType)) : 0; }
    static void Creature_SetPower(Creature* c, uint8 powerType, int32 value) { if (c) c->SetPower(static_cast<Powers>(powerType), value); }
    static float Creature_GetDistanceTo(Creature* c, Unit* target) { return (c && target) ? c->GetDistance(target) : 0.f; }
    static bool Creature_IsWithinDist(Creature* c, Unit* target, float dist) { return (c && target) ? c->IsWithinDist(target, dist) : false; }
    static void Creature_Attack(Creature* c, Unit* target, bool melee) { if (c && target) c->Attack(target, melee); }
    static void Creature_AttackStop(Creature* c) { if (c) c->AttackStop(); }
    static void Creature_Kill(Creature* c, Unit* victim) { if (c && victim) Unit::Kill(c, victim, true); }
    static bool Creature_IsInWorld(Creature* c) { return c ? c->IsInWorld() : false; }
    static bool Creature_IsSummon(Creature* c) { return c ? c->IsSummon() : false; }
    static bool Creature_IsPet(Creature* c) { return c ? c->IsPet() : false; }
    static uint8 Creature_GetClass(Creature* c) { return c ? c->GetClass() : 0; }
    static uint8 Creature_GetRace(Creature* c) { return c ? c->GetRace() : 0; }
    static bool Creature_IsMounted(Creature* c) { return c ? c->IsMounted() : false; }
    static bool Creature_HasUnitFlag(Creature* c, uint32 flag) { return c ? c->HasUnitFlag(static_cast<UnitFlags>(flag)) : false; }
    static void Creature_SetUnitFlag(Creature* c, uint32 flag) { if (c) c->SetUnitFlag(static_cast<UnitFlags>(flag)); }
    static void Creature_RemoveUnitFlag(Creature* c, uint32 flag) { if (c) c->RemoveUnitFlag(static_cast<UnitFlags>(flag)); }

    // Cast methods
    static Player* Creature_ToPlayer(Creature* c) { return c ? c->ToPlayer() : nullptr; }
    static Creature* Creature_Self(Creature* c) { return c; }

    void RegisterCreatureAPI(asIScriptEngine* _scriptEngine)
    {
        int r = _scriptEngine->RegisterObjectType("Creature", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register Creature type: {}", r);
            return;
        }

        // Basic info
        r = _scriptEngine->RegisterObjectMethod("Creature", "string GetName() const", asFUNCTION(Creature_GetName), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint32 GetEntry() const", asFUNCTION(Creature_GetEntry), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint64 GetGUID() const", asFUNCTION(Creature_GetGUID), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint32 GetGUIDLow() const", asFUNCTION(Creature_GetGUIDLow), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint32 GetSpawnId() const", asFUNCTION(Creature_GetSpawnId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint8 GetLevel() const", asFUNCTION(Creature_GetLevel), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint8 GetClass() const", asFUNCTION(Creature_GetClass), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint8 GetRace() const", asFUNCTION(Creature_GetRace), asCALL_CDECL_OBJFIRST);

        // Status
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsAlive() const", asFUNCTION(Creature_IsAlive), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsDead() const", asFUNCTION(Creature_IsDead), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsInCombat() const", asFUNCTION(Creature_IsInCombat), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsInCombatWith(Unit@) const", asFUNCTION(Creature_IsInCombatWith), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool CanHaveThreatList() const", asFUNCTION(Creature_CanHaveThreatList), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsInWorld() const", asFUNCTION(Creature_IsInWorld), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsSummon() const", asFUNCTION(Creature_IsSummon), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsPet() const", asFUNCTION(Creature_IsPet), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsMounted() const", asFUNCTION(Creature_IsMounted), asCALL_CDECL_OBJFIRST);

        // Combat
        r = _scriptEngine->RegisterObjectMethod("Creature", "void SetInCombatWith(Unit@)", asFUNCTION(Creature_SetInCombatWith), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void Attack(Unit@, bool)", asFUNCTION(Creature_Attack), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void AttackStop()", asFUNCTION(Creature_AttackStop), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void Kill(Unit@)", asFUNCTION(Creature_Kill), asCALL_CDECL_OBJFIRST);

        // Health
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint64 GetHealth() const", asFUNCTION(Creature_GetHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint64 GetMaxHealth() const", asFUNCTION(Creature_GetMaxHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void SetHealth(uint64)", asFUNCTION(Creature_SetHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void SetFullHealth()", asFUNCTION(Creature_SetFullHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "float GetHealthPct() const", asFUNCTION(Creature_GetHealthPct), asCALL_CDECL_OBJFIRST);

        // Power
        r = _scriptEngine->RegisterObjectMethod("Creature", "int32 GetPower(uint8) const", asFUNCTION(Creature_GetPower), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "int32 GetMaxPower(uint8) const", asFUNCTION(Creature_GetMaxPower), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void SetPower(uint8, int32)", asFUNCTION(Creature_SetPower), asCALL_CDECL_OBJFIRST);

        // Auras
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool HasAura(uint32) const", asFUNCTION(Creature_HasAura), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint32 GetAuraCount(uint32) const", asFUNCTION(Creature_GetAuraCount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void RemoveAura(uint32)", asFUNCTION(Creature_RemoveAura), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void RemoveAllAuras()", asFUNCTION(Creature_RemoveAllAuras), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void AddAura(uint32, Unit@)", asFUNCTION(Creature_AddAura), asCALL_CDECL_OBJFIRST);

        // Spells
        r = _scriptEngine->RegisterObjectMethod("Creature", "void CastSpell(Unit@, uint32)", asFUNCTIONPR(Creature_CastSpell, (Creature*, Unit*, uint32), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void CastSpell(Unit@, uint32, bool)", asFUNCTIONPR(Creature_CastSpell, (Creature*, Unit*, uint32, bool), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void CastSpell(Unit@, uint32, CastSpellExtraArgs@)", asFUNCTIONPR(Creature_CastSpell, (Creature*, Unit*, uint32, CastSpellExtraArgs const&), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void CastSpell(CastSpellTargetArg@, uint32, CastSpellExtraArgs@)", asFUNCTIONPR(Creature_CastSpell, (Creature*, CastSpellTargetArg const&, uint32, CastSpellExtraArgs const&), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void CastSpellSelf(uint32)", asFUNCTIONPR(Creature_CastSpellSelf, (Creature*, uint32), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void CastSpellSelf(uint32, bool)", asFUNCTIONPR(Creature_CastSpellSelf, (Creature*, uint32, bool), void), asCALL_CDECL_OBJFIRST);

        // React state
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint8 GetReactState() const", asFUNCTION(Creature_GetReactState), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void SetReactState(uint8)", asFUNCTION(Creature_SetReactState), asCALL_CDECL_OBJFIRST);

        // Loot
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool HasLootRecipient() const", asFUNCTION(Creature_HasLootRecipient), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsTappedBy(Player@) const", asFUNCTION(Creature_IsTappedBy), asCALL_CDECL_OBJFIRST);

        // Quest
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool HasQuest(uint32) const", asFUNCTION(Creature_HasQuest), asCALL_CDECL_OBJFIRST);

        // Spawn/Despawn
        r = _scriptEngine->RegisterObjectMethod("Creature", "void Respawn(bool)", asFUNCTION(Creature_Respawn), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void DespawnOrUnsummon(uint32)", asFUNCTION(Creature_DespawnOrUnsummon), asCALL_CDECL_OBJFIRST);

        // Faction
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint32 GetFaction() const", asFUNCTION(Creature_GetFaction), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsFriendlyTo(Unit@) const", asFUNCTION(Creature_IsFriendlyTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsHostileTo(Unit@) const", asFUNCTION(Creature_IsHostileTo), asCALL_CDECL_OBJFIRST);

        // Position
        r = _scriptEngine->RegisterObjectMethod("Creature", "uint32 GetMapId() const", asFUNCTION(Creature_GetMapId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "float GetPositionX() const", asFUNCTION(Creature_GetPositionX), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "float GetPositionY() const", asFUNCTION(Creature_GetPositionY), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "float GetPositionZ() const", asFUNCTION(Creature_GetPositionZ), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "float GetOrientation() const", asFUNCTION(Creature_GetOrientation), asCALL_CDECL_OBJFIRST);

        // Distance
        r = _scriptEngine->RegisterObjectMethod("Creature", "float GetDistanceTo(Unit@) const", asFUNCTION(Creature_GetDistanceTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool IsWithinDist(Unit@, float) const", asFUNCTION(Creature_IsWithinDist), asCALL_CDECL_OBJFIRST);

        // Flags
        r = _scriptEngine->RegisterObjectMethod("Creature", "bool HasUnitFlag(uint32) const", asFUNCTION(Creature_HasUnitFlag), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void SetUnitFlag(uint32)", asFUNCTION(Creature_SetUnitFlag), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "void RemoveUnitFlag(uint32)", asFUNCTION(Creature_RemoveUnitFlag), asCALL_CDECL_OBJFIRST);

        // Cast
        r = _scriptEngine->RegisterObjectMethod("Creature", "Player@ ToPlayer() const", asFUNCTION(Creature_ToPlayer), asCALL_CDECL_OBJFIRST);

        TC_LOG_INFO("server.angelscript", "Creature API registered ({} methods)", 55);
    }

} // namespace AngelScript
