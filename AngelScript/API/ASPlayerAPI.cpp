/*
 * AngelScript Player API
 * Wrapper functions and type registration for Player
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

#include "Player.h"
#include "Creature.h"
#include "Unit.h"
#include "WorldSession.h"
#include "Item.h"
#include "QuestDef.h"
#include "ReputationMgr.h"
#include "Chat.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "ObjectMgr.h"
#include "CollectionMgr.h"
#include "Log.h"

namespace AngelScript
{
    // ---- Player wrapper functions ----
    static std::string Player_GetName(Player* p) { return p ? p->GetName() : ""; }
    static uint64 Player_GetGUID(Player* p) { return p ? p->GetGUID().GetRawValue(0) : 0; }
    static uint32 Player_GetGUIDLow(Player* p) { return p ? static_cast<uint32>(p->GetGUID().GetCounter()) : 0; }
    static uint8 Player_GetLevel(Player* p) { return p ? p->GetLevel() : 0; }
    static uint8 Player_GetClass(Player* p) { return p ? p->GetClass() : 0; }
    static uint8 Player_GetRace(Player* p) { return p ? p->GetRace() : 0; }
    static uint8 Player_GetGender(Player* p) { return p ? static_cast<uint8>(p->GetGender()) : 0; }
    static bool Player_IsAlive(Player* p) { return p ? p->IsAlive() : false; }
    static bool Player_IsInCombat(Player* p) { return p ? p->IsInCombat() : false; }
    static bool Player_IsInWorld(Player* p) { return p ? p->IsInWorld() : false; }
    static bool Player_IsAFK(Player* p) { return p ? p->isAFK() : false; }
    static bool Player_IsDND(Player* p) { return p ? p->isDND() : false; }
    static bool Player_IsGM(Player* p) { return p ? p->IsGameMaster() : false; }
    static void Player_SetGM(Player* p, bool on) { if (p) p->SetGameMaster(on); }
    static bool Player_IsMounted(Player* p) { return p ? p->IsMounted() : false; }
    static uint32 Player_GetMapId(Player* p) { return p ? p->GetMapId() : 0; }
    static uint32 Player_GetZoneId(Player* p) { return p ? p->GetZoneId() : 0; }
    static uint32 Player_GetAreaId(Player* p) { return p ? p->GetAreaId() : 0; }
    static float Player_GetPositionX(Player* p) { return p ? p->GetPositionX() : 0.f; }
    static float Player_GetPositionY(Player* p) { return p ? p->GetPositionY() : 0.f; }
    static float Player_GetPositionZ(Player* p) { return p ? p->GetPositionZ() : 0.f; }
    static float Player_GetOrientation(Player* p) { return p ? p->GetOrientation() : 0.f; }
    static uint64 Player_GetHealth(Player* p) { return p ? p->GetHealth() : 0; }
    static uint64 Player_GetMaxHealth(Player* p) { return p ? p->GetMaxHealth() : 0; }
    static float Player_GetHealthPct(Player* p) { return p ? p->GetHealthPct() : 0.f; }
    static uint64 Player_GetMoney(Player* p) { return p ? p->GetMoney() : 0; }
    static bool Player_ModifyMoney(Player* p, int64 amount) { return p ? p->ModifyMoney(amount) : false; }
    static uint32 Player_GetItemCount(Player* p, uint32 item, bool inBank) { return p ? p->GetItemCount(item, inBank) : 0; }
    static bool Player_HasItemCount(Player* p, uint32 item, uint32 count) { return p ? p->HasItemCount(item, count) : false; }
    static bool Player_StoreNewItemInBestSlots(Player* p, uint32 itemId, uint32 count) { if (!p) return false; p->StoreNewItemInBestSlots(itemId, count, ItemContext::NONE); return true; }
    static uint32 Player_GetQuestStatus(Player* p, uint32 questId) { return p ? static_cast<uint32>(p->GetQuestStatus(questId)) : 0; }
    static bool Player_IsQuestRewarded(Player* p, uint32 questId) { return p ? p->IsQuestRewarded(questId) : false; }
    static void Player_CompleteQuest(Player* p, uint32 questId) { if (p) p->CompleteQuest(questId); }
    static void Player_FailQuest(Player* p, uint32 questId) { if (p) p->FailQuest(questId); }
    static int32 Player_GetReputation(Player* p, uint32 factionId) { return p ? p->GetReputation(factionId) : 0; }
    static void Player_SetReputation(Player* p, uint32 factionId, int32 value) { if (p) p->SetReputation(factionId, value); }
    static uint32 Player_GetGuildId(Player* p) { return p ? static_cast<uint32>(p->GetGuildId()) : 0; }
    static uint32 Player_GetAccountId(Player* p)           { return (p && p->GetSession()) ? p->GetSession()->GetAccountId() : 0; }
    static uint32 Player_GetBattlenetAccountId(Player* p)  { return (p && p->GetSession()) ? p->GetSession()->GetBattlenetAccountId() : 0; }
    static std::string Player_GetAccountName(Player* p)    { return (p && p->GetSession()) ? p->GetSession()->GetAccountName() : ""; }
    static void Player_SendNotification(Player* p, const std::string& msg) { if (p && p->GetSession()) p->GetSession()->SendNotification("%s", msg.c_str()); }
    static bool Player_HasAura(Player* p, uint32 spellId) { return p ? p->HasAura(spellId) : false; }
    static bool Player_TeleportTo(Player* p, uint32 mapId, float x, float y, float z, float o) { return p ? p->TeleportTo(mapId, x, y, z, o) : false; }
    // Relocate: changes position in-memory WITHOUT a loading screen (same map only!)
    // For cross-map movement use TeleportTo instead.
    static void Player_Relocate(Player* p, float x, float y, float z, float o) { if (p) p->Relocate(x, y, z, o); }
    static void Player_Relocate(Player* p, float x, float y, float z) { if (p) p->Relocate(x, y, z); }

    // ---- NEW: Missing critical API wrappers ----
    static uint32 Player_GetTeam(Player* p) { return p ? static_cast<uint32>(p->GetTeam()) : 0; }
    static void Player_CastSpell(Player* p, Unit* target, uint32 spellId) { if (p && target) p->CastSpell(target, spellId, true); }
    static void Player_CastSpell(Player* p, Unit* target, uint32 spellId, bool triggered) { if (p && target) p->CastSpell(target, spellId, triggered); }
    static void Player_CastSpell(Player* p, Unit* target, uint32 spellId, CastSpellExtraArgs const& args) { if (p && target) p->CastSpell(target, spellId, args); }
    static void Player_CastSpell(Player* p, CastSpellTargetArg const& targetArg, uint32 spellId, CastSpellExtraArgs const& args) { if (p) p->CastSpell(targetArg, spellId, args); }
    static bool Player_LearnSpell(Player* p, uint32 spellId) { if (!p) return false; p->LearnSpell(spellId, false); return true; }
    static bool Player_HasSpell(Player* p, uint32 spellId) { return p ? p->HasSpell(spellId) : false; }
    static uint32 Player_GetFreeInventorySlotCount(Player* p) { return p ? p->GetFreeInventorySlotCount() : 0; }

    // ---- CollectionMgr wrappers for BattlePay delivery ----
    static bool Player_AddMount(Player* p, uint32 spellId)
    {
        if (!p || !p->GetSession() || !p->GetSession()->GetCollectionMgr())
            return false;
        return p->GetSession()->GetCollectionMgr()->AddMount(spellId, MountStatusFlags::MOUNT_STATUS_NONE, false, false);
    }

    static bool Player_HasMount(Player* p, uint32 spellId)
    {
        if (!p || !p->GetSession() || !p->GetSession()->GetCollectionMgr())
            return false;
        auto const& mounts = p->GetSession()->GetCollectionMgr()->GetAccountMounts();
        return mounts.find(spellId) != mounts.end();
    }

    static bool Player_AddToy(Player* p, uint32 itemId)
    {
        if (!p || !p->GetSession() || !p->GetSession()->GetCollectionMgr())
            return false;
        return p->GetSession()->GetCollectionMgr()->AddToy(itemId, false, false);
    }

    static bool Player_HasToy(Player* p, uint32 itemId)
    {
        if (!p || !p->GetSession() || !p->GetSession()->GetCollectionMgr())
            return false;
        return p->GetSession()->GetCollectionMgr()->HasToy(itemId);
    }

    static void Player_AddTransmogSet(Player* p, uint32 transmogSetId)
    {
        if (!p || !p->GetSession() || !p->GetSession()->GetCollectionMgr())
            return;
        p->GetSession()->GetCollectionMgr()->AddTransmogSet(transmogSetId);
    }

    static void Player_CastSpellSelf(Player* p, uint32 spellId) { if (p) p->CastSpell(p, spellId, true); }
    static void Player_CastSpellSelf(Player* p, uint32 spellId, bool triggered) { if (p) p->CastSpell(p, spellId, triggered); }
    static void Player_AddAura(Player* p, uint32 spellId, Unit* caster) { if (p && caster) p->AddAura(spellId, caster); }
    static void Player_RemoveAura(Player* p, uint32 spellId) { if (p) p->RemoveAura(spellId); }
    static void Player_GiveXP(Player* p, uint32 amount, Unit* victim) { if (p) p->GiveXP(amount, victim); }
    static void Player_SetHealth(Player* p, uint64 health) { if (p) p->SetHealth(health); }
    static void Player_SetFullHealth(Player* p) { if (p) p->SetFullHealth(); }
    static int32 Player_GetPower(Player* p, uint8 powerType) { return p ? p->GetPower(static_cast<Powers>(powerType)) : 0; }
    static int32 Player_GetMaxPower(Player* p, uint8 powerType) { return p ? p->GetMaxPower(static_cast<Powers>(powerType)) : 0; }
    static void Player_SetPower(Player* p, uint8 powerType, int32 value) { if (p) p->SetPower(static_cast<Powers>(powerType), value); }
    static uint32 Player_GetAuraCount(Player* p, uint32 spellId) { return p ? p->GetAuraCount(spellId) : 0; }
    static void Player_RemoveAllAuras(Player* p) { if (p) p->RemoveAllAuras(); }
    static uint32 Player_GetFaction(Player* p) { return p ? p->GetFaction() : 0; }
    static bool Player_IsFriendlyTo(Player* p, Unit* target) { return (p && target) ? p->IsFriendlyTo(target) : false; }
    static bool Player_IsHostileTo(Player* p, Unit* target) { return (p && target) ? p->IsHostileTo(target) : false; }
    static float Player_GetDistanceTo(Player* p, Unit* target) { return (p && target) ? p->GetDistance(target) : 0.f; }
    static bool Player_IsWithinDist(Player* p, Unit* target, float dist) { return (p && target) ? p->IsWithinDist(target, dist) : false; }
    static bool Player_HasUnitFlag(Player* p, uint32 flag) { return p ? p->HasUnitFlag(static_cast<UnitFlags>(flag)) : false; }
    static void Player_Kill(Player* p, Unit* victim) { if (p && victim) Unit::Kill(p, victim, true); }
    static bool Player_IsDead(Player* p) { return p ? p->isDead() : false; }
    static bool Player_HasQuest(Player* p, uint32 questId) { return p ? p->IsActiveQuest(questId) : false; }
    static uint32 Player_GetSkillStep(Player* p, uint32 skillId) { return p ? p->GetSkillStep(skillId) : 0; }
    static uint16 Player_GetSkillValue(Player* p, uint32 skillId) { return p ? p->GetSkillValue(skillId) : 0; }
    static void Player_SendAreaTriggerMessage(Player* p, const std::string& msg) { if (p && p->GetSession()) p->GetSession()->SendNotification("%s", msg.c_str()); }
    static void Player_UpdateZone(Player* p, uint32 newZone, uint32 newArea) { if (p) p->UpdateZone(newZone, newArea); }
    static void Player_AddMoney(Player* p, uint64 amount) { if (p) p->ModifyMoney(int64(amount)); }

    // Cast methods — critical for type dispatch
    static Creature* Player_ToCreature(Player* /*p*/) { return nullptr; } // Player cannot be creature
    static Player* Player_Self(Player* p) { return p; } // Identity cast

    void RegisterPlayerAPI(asIScriptEngine* _scriptEngine)
    {
        int r = _scriptEngine->RegisterObjectType("Player", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register Player type: {}", r);
            return;
        }

        // Basic info
        r = _scriptEngine->RegisterObjectMethod("Player", "string GetName() const", asFUNCTION(Player_GetName), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint64 GetGUID() const", asFUNCTION(Player_GetGUID), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetGUIDLow() const", asFUNCTION(Player_GetGUIDLow), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint8 GetLevel() const", asFUNCTION(Player_GetLevel), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint8 GetClass() const", asFUNCTION(Player_GetClass), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint8 GetRace() const", asFUNCTION(Player_GetRace), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint8 GetGender() const", asFUNCTION(Player_GetGender), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetTeam() const", asFUNCTION(Player_GetTeam), asCALL_CDECL_OBJFIRST);

        // Communication
        r = _scriptEngine->RegisterObjectMethod("Player", "void SendNotification(const string& in)", asFUNCTION(Player_SendNotification), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void SendAreaTriggerMessage(const string& in)", asFUNCTION(Player_SendAreaTriggerMessage), asCALL_CDECL_OBJFIRST);

        // Status
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsAlive() const", asFUNCTION(Player_IsAlive), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsDead() const", asFUNCTION(Player_IsDead), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsInCombat() const", asFUNCTION(Player_IsInCombat), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsOnline() const", asFUNCTION(Player_IsInWorld), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsAFK() const", asFUNCTION(Player_IsAFK), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsDND() const", asFUNCTION(Player_IsDND), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsGM() const", asFUNCTION(Player_IsGM), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void SetGM(bool)", asFUNCTION(Player_SetGM), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsMounted() const", asFUNCTION(Player_IsMounted), asCALL_CDECL_OBJFIRST);

        // Position
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetMapId() const", asFUNCTION(Player_GetMapId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetZoneId() const", asFUNCTION(Player_GetZoneId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetAreaId() const", asFUNCTION(Player_GetAreaId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "float GetPositionX() const", asFUNCTION(Player_GetPositionX), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "float GetPositionY() const", asFUNCTION(Player_GetPositionY), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "float GetPositionZ() const", asFUNCTION(Player_GetPositionZ), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "float GetOrientation() const", asFUNCTION(Player_GetOrientation), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool TeleportTo(uint32, float, float, float, float)", asFUNCTION(Player_TeleportTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void Relocate(float, float, float, float)", asFUNCTIONPR(Player_Relocate, (Player*, float, float, float, float), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void Relocate(float, float, float)", asFUNCTIONPR(Player_Relocate, (Player*, float, float, float), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void UpdateZone(uint32, uint32)", asFUNCTION(Player_UpdateZone), asCALL_CDECL_OBJFIRST);

        // Health
        r = _scriptEngine->RegisterObjectMethod("Player", "uint64 GetHealth() const", asFUNCTION(Player_GetHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint64 GetMaxHealth() const", asFUNCTION(Player_GetMaxHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void SetHealth(uint64)", asFUNCTION(Player_SetHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void SetFullHealth()", asFUNCTION(Player_SetFullHealth), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "float GetHealthPct() const", asFUNCTION(Player_GetHealthPct), asCALL_CDECL_OBJFIRST);

        // Power
        r = _scriptEngine->RegisterObjectMethod("Player", "int32 GetPower(uint8) const", asFUNCTION(Player_GetPower), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "int32 GetMaxPower(uint8) const", asFUNCTION(Player_GetMaxPower), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void SetPower(uint8, int32)", asFUNCTION(Player_SetPower), asCALL_CDECL_OBJFIRST);

        // Money
        r = _scriptEngine->RegisterObjectMethod("Player", "uint64 GetMoney() const", asFUNCTION(Player_GetMoney), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool ModifyMoney(int64)", asFUNCTION(Player_ModifyMoney), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void AddMoney(uint64)", asFUNCTION(Player_AddMoney), asCALL_CDECL_OBJFIRST);

        // Inventory
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetItemCount(uint32, bool) const", asFUNCTION(Player_GetItemCount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasItemCount(uint32, uint32) const", asFUNCTION(Player_HasItemCount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool AddItem(uint32, uint32)", asFUNCTION(Player_StoreNewItemInBestSlots), asCALL_CDECL_OBJFIRST);

        // Quests
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetQuestStatus(uint32) const", asFUNCTION(Player_GetQuestStatus), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsQuestRewarded(uint32) const", asFUNCTION(Player_IsQuestRewarded), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void CompleteQuest(uint32)", asFUNCTION(Player_CompleteQuest), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void FailQuest(uint32)", asFUNCTION(Player_FailQuest), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasQuest(uint32) const", asFUNCTION(Player_HasQuest), asCALL_CDECL_OBJFIRST);

        // Reputation
        r = _scriptEngine->RegisterObjectMethod("Player", "int32 GetReputation(uint32) const", asFUNCTION(Player_GetReputation), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void SetReputation(uint32, int32)", asFUNCTION(Player_SetReputation), asCALL_CDECL_OBJFIRST);

        // Guild
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetGuildId() const", asFUNCTION(Player_GetGuildId), asCALL_CDECL_OBJFIRST);

        // Session
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetAccountId() const",          asFUNCTION(Player_GetAccountId),          asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetBattlenetAccountId() const",  asFUNCTION(Player_GetBattlenetAccountId), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "string GetAccountName() const",         asFUNCTION(Player_GetAccountName),        asCALL_CDECL_OBJFIRST);

        // Auras
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasAura(uint32) const", asFUNCTION(Player_HasAura), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetAuraCount(uint32) const", asFUNCTION(Player_GetAuraCount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void RemoveAura(uint32)", asFUNCTION(Player_RemoveAura), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void RemoveAllAuras()", asFUNCTION(Player_RemoveAllAuras), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void AddAura(uint32, Unit@)", asFUNCTION(Player_AddAura), asCALL_CDECL_OBJFIRST);

        // Spells
        r = _scriptEngine->RegisterObjectMethod("Player", "void CastSpell(Unit@, uint32)", asFUNCTIONPR(Player_CastSpell, (Player*, Unit*, uint32), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void CastSpell(Unit@, uint32, bool)", asFUNCTIONPR(Player_CastSpell, (Player*, Unit*, uint32, bool), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void CastSpell(Unit@, uint32, CastSpellExtraArgs@)", asFUNCTIONPR(Player_CastSpell, (Player*, Unit*, uint32, CastSpellExtraArgs const&), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void CastSpell(CastSpellTargetArg@, uint32, CastSpellExtraArgs@)", asFUNCTIONPR(Player_CastSpell, (Player*, CastSpellTargetArg const&, uint32, CastSpellExtraArgs const&), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void CastSpellSelf(uint32)", asFUNCTIONPR(Player_CastSpellSelf, (Player*, uint32), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void CastSpellSelf(uint32, bool)", asFUNCTIONPR(Player_CastSpellSelf, (Player*, uint32, bool), void), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool LearnSpell(uint32)", asFUNCTION(Player_LearnSpell), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasSpell(uint32) const", asFUNCTION(Player_HasSpell), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetFreeInventorySlotCount() const", asFUNCTION(Player_GetFreeInventorySlotCount), asCALL_CDECL_OBJFIRST);

        // CollectionMgr (BattlePay delivery)
        r = _scriptEngine->RegisterObjectMethod("Player", "bool AddMount(uint32)", asFUNCTION(Player_AddMount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasMount(uint32) const", asFUNCTION(Player_HasMount), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool AddToy(uint32)", asFUNCTION(Player_AddToy), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasToy(uint32) const", asFUNCTION(Player_HasToy), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "void AddTransmogSet(uint32)", asFUNCTION(Player_AddTransmogSet), asCALL_CDECL_OBJFIRST);

        // XP
        r = _scriptEngine->RegisterObjectMethod("Player", "void GiveXP(uint32, Unit@)", asFUNCTION(Player_GiveXP), asCALL_CDECL_OBJFIRST);

        // Faction
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetFaction() const", asFUNCTION(Player_GetFaction), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsFriendlyTo(Unit@) const", asFUNCTION(Player_IsFriendlyTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsHostileTo(Unit@) const", asFUNCTION(Player_IsHostileTo), asCALL_CDECL_OBJFIRST);

        // Flags
        r = _scriptEngine->RegisterObjectMethod("Player", "bool HasUnitFlag(uint32) const", asFUNCTION(Player_HasUnitFlag), asCALL_CDECL_OBJFIRST);

        // Distance
        r = _scriptEngine->RegisterObjectMethod("Player", "float GetDistanceTo(Unit@) const", asFUNCTION(Player_GetDistanceTo), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "bool IsWithinDist(Unit@, float) const", asFUNCTION(Player_IsWithinDist), asCALL_CDECL_OBJFIRST);

        // Kill
        r = _scriptEngine->RegisterObjectMethod("Player", "void Kill(Unit@)", asFUNCTION(Player_Kill), asCALL_CDECL_OBJFIRST);

        // Skills
        r = _scriptEngine->RegisterObjectMethod("Player", "uint32 GetSkillStep(uint32) const", asFUNCTION(Player_GetSkillStep), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "uint16 GetSkillValue(uint32) const", asFUNCTION(Player_GetSkillValue), asCALL_CDECL_OBJFIRST);

        TC_LOG_INFO("server.angelscript", "Player API registered ({} methods)", 65);
    }

} // namespace AngelScript
