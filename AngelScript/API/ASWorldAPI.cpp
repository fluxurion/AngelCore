/*
 * AngelScript World API
 * Exposes: World functions, phase system, update field access,
 * creature summoning, ObjectMgr access, SendPlayerChoice, hook points
 */

#ifndef ANGELSCRIPT_INTEGRATION
    #error "ANGELSCRIPT_INTEGRATION macro must be defined"
#endif

#include "ASWorldAPI.h"
#include "ASHookTypes.h"
#include "AngelScriptMgr.h"
#include "angelscript.h"

#include "World.h"
#include "Player.h"
#include "Creature.h"
#include "TemporarySummon.h"
#include "GameObject.h"
#include "Unit.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "Chat.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "PlayerChoice.h"
#include "Log.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "ItemTemplate.h"
#include "CreatureData.h"
#include "GameObjectData.h"

namespace AngelScript
{
    // ========================================================================
    // Phase System
    // ========================================================================

    static void Phase_Add(WorldObject* obj, uint32 phaseId, bool updateVisibility)
    {
        if (!obj) return;
        PhasingHandler::AddPhase(obj, phaseId, updateVisibility);
    }

    static void Phase_Remove(WorldObject* obj, uint32 phaseId, bool updateVisibility)
    {
        if (!obj) return;
        PhasingHandler::RemovePhase(obj, phaseId, updateVisibility);
    }

    static void Phase_AddGroup(WorldObject* obj, uint32 phaseGroupId, bool updateVisibility)
    {
        if (!obj) return;
        PhasingHandler::AddPhaseGroup(obj, phaseGroupId, updateVisibility);
    }

    static void Phase_RemoveGroup(WorldObject* obj, uint32 phaseGroupId, bool updateVisibility)
    {
        if (!obj) return;
        PhasingHandler::RemovePhaseGroup(obj, phaseGroupId, updateVisibility);
    }

    static void Phase_Reset(WorldObject* obj)
    {
        if (!obj) return;
        PhasingHandler::ResetPhaseShift(obj);
    }

    static bool Phase_Has(WorldObject* obj, uint32 phaseId)
    {
        if (!obj) return false;
        return obj->GetPhaseShift().HasPhase(phaseId);
    }

    // ========================================================================
    // Creature Summoning
    // ========================================================================

    static Creature* SummonCreature(Unit* summoner, uint32 entry, float x, float y, float z, float o, uint32 summonType, uint32 despawnTimer)
    {
        if (!summoner) return nullptr;
        TempSummonType type = static_cast<TempSummonType>(summonType);
        Milliseconds despawnMs(despawnTimer);
        TempSummon* summon = summoner->SummonCreature(entry, Position(x, y, z, o), type, despawnMs);
        return summon; // TempSummon derives from Creature, implicit conversion works
    }

    static Creature* SummonCreatureAtLoc(uint32 /*entry*/, uint32 mapId, float /*x*/, float /*y*/, float /*z*/, float /*o*/, uint32 /*summonType*/, uint32 despawnTimer)
    {
        Map* map = sMapMgr->FindMap(mapId, 0);
        if (!map) return nullptr;
        Milliseconds despawnMs(despawnTimer);
        // Summon via map - need a summoner; use the map's world object
        return nullptr; // TODO: implement map-level summoning
    }

    // ========================================================================
    // ObjectMgr Access
    // ========================================================================

    static std::string ObjectMgr_GetCreatureName(uint32 entry)
    {
        CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(entry);
        return tmpl ? tmpl->Name : "";
    }

    static uint32 ObjectMgr_GetCreatureFaction(uint32 entry)
    {
        CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(entry);
        return tmpl ? tmpl->faction : 0;
    }

    static uint32 ObjectMgr_GetCreatureLevel(uint32 /*entry*/)
    {
        // Creature level is determined by difficulty settings, not template directly
        // Return 0 as level is dynamic based on spawn
        return 0;
    }

    static std::string ObjectMgr_GetGameObjectName(uint32 entry)
    {
        GameObjectTemplate const* tmpl = sObjectMgr->GetGameObjectTemplate(entry);
        return tmpl ? tmpl->name : "";
    }

    static uint32 ObjectMgr_GetGameObjectType(uint32 entry)
    {
        GameObjectTemplate const* tmpl = sObjectMgr->GetGameObjectTemplate(entry);
        return tmpl ? static_cast<uint32>(tmpl->type) : 0;
    }

    static std::string ObjectMgr_GetItemName(uint32 entry)
    {
        ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(entry);
        return tmpl ? tmpl->GetDefaultLocaleName() : "";
    }

    static uint32 ObjectMgr_GetItemClass(uint32 entry)
    {
        ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(entry);
        return tmpl ? tmpl->GetClass() : 0;
    }

    static uint32 ObjectMgr_GetItemSubClass(uint32 entry)
    {
        ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(entry);
        return tmpl ? tmpl->GetSubClass() : 0;
    }

    static uint32 ObjectMgr_GetItemQuality(uint32 entry)
    {
        ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(entry);
        return tmpl ? tmpl->GetQuality() : 0;
    }

    static std::string ObjectMgr_GetQuestTitle(uint32 questId)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        return quest ? quest->GetLogTitle() : "";
    }

    static bool ObjectMgr_HasCreatureTemplate(uint32 entry)
    {
        return sObjectMgr->GetCreatureTemplate(entry) != nullptr;
    }

    static bool ObjectMgr_HasGameObjectTemplate(uint32 entry)
    {
        return sObjectMgr->GetGameObjectTemplate(entry) != nullptr;
    }

    static bool ObjectMgr_HasItemTemplate(uint32 entry)
    {
        return sObjectMgr->GetItemTemplate(entry) != nullptr;
    }

    static bool ObjectMgr_HasQuestTemplate(uint32 questId)
    {
        return sObjectMgr->GetQuestTemplate(questId) != nullptr;
    }

    // ========================================================================
    // SendPlayerChoice
    // ========================================================================

    static void SendPlayerChoice(Player* player, int32 choiceId)
    {
        if (!player) return;
        player->SendPlayerChoice(player->GetGUID(), choiceId);
    }

    static bool HasPlayerChoice(int32 choiceId)
    {
        return sObjectMgr->GetPlayerChoice(choiceId) != nullptr;
    }

    // ========================================================================
    // World Config Access
    // ========================================================================

    static int32 World_GetIntConfig(uint32 config) { return sWorld->getIntConfig(static_cast<WorldIntConfigs>(config)); }
    static float World_GetFloatConfig(uint32 config) { return sWorld->getFloatConfig(static_cast<WorldFloatConfigs>(config)); }
    static bool World_GetBoolConfig(uint32 config) { return sWorld->getBoolConfig(static_cast<WorldBoolConfigs>(config)); }

    // ========================================================================
    // Object Accessor
    // ========================================================================

    static Player* FindPlayerByName(const std::string& name) { return ObjectAccessor::FindPlayerByName(name); }
    static Creature* GetCreatureByGuid(Unit* unit, uint64 guidLow) { 
        if (!unit) return nullptr; 
        // ObjectGuid::Create for WorldObject requires mapId, entry, and counter
        // We need to use GetCreature with map lookup instead
        return unit->GetMap()->GetCreatureBySpawnId(guidLow); 
    }
    static GameObject* GetGameObjectByGuid(Unit* unit, uint64 guidLow) { 
        if (!unit) return nullptr; 
        return unit->GetMap()->GetGameObjectBySpawnId(guidLow); 
    }

    // ========================================================================
    // Hook Point System
    // ========================================================================
    // Hook points allow AngelScript to intercept specific TC functions.
    // To add a new hook point:
    //   1. Add a hook type enum in ASHookTypes.h
    //   2. Add a Trigger function in AngelScriptMgr.h/cpp
    //   3. Add a Register function in AngelScriptMgr.h/cpp
    //   4. Patch the TC function to call the trigger
    //   5. Register the AngelScript function signature here

    static void RegisterCustomHook_SendPlayerChoice(int hookType, asIScriptFunction* func)
    {
        sAngelScriptMgr->RegisterCustomHook(static_cast<CustomHookType>(hookType), func);
    }

    static void RegisterCustomHook_AuthResponse(asIScriptFunction* func)
    {
        sAngelScriptMgr->RegisterCustomHook(CustomHookType::ON_AUTH_RESPONSE, func);
    }

    static void RegisterCustomHook_FeatureSystemStatusGlueScreen(asIScriptFunction* func)
    {
        sAngelScriptMgr->RegisterCustomHook(CustomHookType::ON_FEATURE_SYSTEM_STATUS_GLUE_SCREEN, func);
    }

    // ========================================================================
    // Registration
    // ========================================================================

    void RegisterWorldAPI(asIScriptEngine* _scriptEngine)
    {
        int r;

        // ---- WorldObject type ----
        // WorldObject is the base class for Unit, Creature, Player, GameObject
        // AngelScript supports interface inheritance via implicit casts
        r = _scriptEngine->RegisterObjectType("WorldObject", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register WorldObject type: {}", r);
            return;
        }

        // Register implicit casts from derived types to base types
        // opImplCast enables automatic conversion (e.g. Player@ → Unit@ in function args)
        // opCast is explicit only (requires cast<T>(obj))

        // Player -> Unit, Creature -> Unit, GameObject -> Unit (derived to Unit)
        r = _scriptEngine->RegisterObjectMethod("Player", "Unit@ opImplCast()", asFUNCTION((+[](Player* p) -> Unit* { return p; })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "Unit@ opImplCast()", asFUNCTION((+[](Creature* c) -> Unit* { return c; })), asCALL_CDECL_OBJFIRST);

        // Unit -> WorldObject, Player -> WorldObject, Creature -> WorldObject, GameObject -> WorldObject
        r = _scriptEngine->RegisterObjectMethod("Unit", "WorldObject@ opImplCast()", asFUNCTION((+[](Unit* u) -> WorldObject* { return u; })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Player", "WorldObject@ opImplCast()", asFUNCTION((+[](Player* p) -> WorldObject* { return p; })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("Creature", "WorldObject@ opImplCast()", asFUNCTION((+[](Creature* c) -> WorldObject* { return c; })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("GameObject", "WorldObject@ opImplCast()", asFUNCTION((+[](GameObject* g) -> WorldObject* { return g; })), asCALL_CDECL_OBJFIRST);

        // WorldObject -> derived (explicit cast only — not all WorldObjects are Players)
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "Unit@ opCast()", asFUNCTION((+[](WorldObject* w) -> Unit* { return w->ToUnit(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "Player@ opCast()", asFUNCTION((+[](WorldObject* w) -> Player* { return w->ToPlayer(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "Creature@ opCast()", asFUNCTION((+[](WorldObject* w) -> Creature* { return w->ToCreature(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "GameObject@ opCast()", asFUNCTION((+[](WorldObject* w) -> GameObject* { return w->ToGameObject(); })), asCALL_CDECL_OBJFIRST);

        // WorldObject methods
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "uint32 GetMapId() const", asFUNCTION((+[](WorldObject* w) -> uint32 { return w->GetMapId(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "uint32 GetZoneId() const", asFUNCTION((+[](WorldObject* w) -> uint32 { return w->GetZoneId(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "uint32 GetAreaId() const", asFUNCTION((+[](WorldObject* w) -> uint32 { return w->GetAreaId(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "float GetPositionX() const", asFUNCTION((+[](WorldObject* w) -> float { return w->GetPositionX(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "float GetPositionY() const", asFUNCTION((+[](WorldObject* w) -> float { return w->GetPositionY(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "float GetPositionZ() const", asFUNCTION((+[](WorldObject* w) -> float { return w->GetPositionZ(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "float GetOrientation() const", asFUNCTION((+[](WorldObject* w) -> float { return w->GetOrientation(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "uint64 GetGUID() const", asFUNCTION((+[](WorldObject* w) -> uint64 { return w->GetGUID().GetCounter(); })), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("WorldObject", "string GetName() const", asFUNCTION((+[](WorldObject* w) -> std::string { return w->GetName(); })), asCALL_CDECL_OBJFIRST);

        // ---- Phase System ----
        r = _scriptEngine->RegisterGlobalFunction("void PhaseAdd(WorldObject@, uint32, bool)", asFUNCTION(Phase_Add), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void PhaseRemove(WorldObject@, uint32, bool)", asFUNCTION(Phase_Remove), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void PhaseAddGroup(WorldObject@, uint32, bool)", asFUNCTION(Phase_AddGroup), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void PhaseRemoveGroup(WorldObject@, uint32, bool)", asFUNCTION(Phase_RemoveGroup), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void PhaseReset(WorldObject@)", asFUNCTION(Phase_Reset), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool PhaseHas(WorldObject@, uint32)", asFUNCTION(Phase_Has), asCALL_CDECL);

        // ---- Creature Summoning ----
        r = _scriptEngine->RegisterGlobalFunction("Creature@ SummonCreature(Unit@, uint32, float, float, float, float, uint32, uint32)", asFUNCTION(SummonCreature), asCALL_CDECL);

        // ---- ObjectMgr Access ----
        r = _scriptEngine->RegisterGlobalFunction("string GetCreatureTemplateName(uint32)", asFUNCTION(ObjectMgr_GetCreatureName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetCreatureTemplateFaction(uint32)", asFUNCTION(ObjectMgr_GetCreatureFaction), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetCreatureTemplateLevel(uint32)", asFUNCTION(ObjectMgr_GetCreatureLevel), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetGameObjectTemplateName(uint32)", asFUNCTION(ObjectMgr_GetGameObjectName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetGameObjectTemplateType(uint32)", asFUNCTION(ObjectMgr_GetGameObjectType), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetItemTemplateName(uint32)", asFUNCTION(ObjectMgr_GetItemName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetItemTemplateQuality(uint32)", asFUNCTION(ObjectMgr_GetItemQuality), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetQuestTitle(uint32)", asFUNCTION(ObjectMgr_GetQuestTitle), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasCreatureTemplate(uint32)", asFUNCTION(ObjectMgr_HasCreatureTemplate), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasGameObjectTemplate(uint32)", asFUNCTION(ObjectMgr_HasGameObjectTemplate), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasItemTemplate(uint32)", asFUNCTION(ObjectMgr_HasItemTemplate), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasQuestTemplate(uint32)", asFUNCTION(ObjectMgr_HasQuestTemplate), asCALL_CDECL);

        // ---- SendPlayerChoice ----
        r = _scriptEngine->RegisterGlobalFunction("void SendPlayerChoice(Player@, int32)", asFUNCTION(SendPlayerChoice), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasPlayerChoice(int32)", asFUNCTION(HasPlayerChoice), asCALL_CDECL);

        // ---- World Config ----
        r = _scriptEngine->RegisterGlobalFunction("int32 GetWorldIntConfig(uint32)", asFUNCTION(World_GetIntConfig), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("float GetWorldFloatConfig(uint32)", asFUNCTION(World_GetFloatConfig), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool GetWorldBoolConfig(uint32)", asFUNCTION(World_GetBoolConfig), asCALL_CDECL);

        // ---- Object Accessor ----
        r = _scriptEngine->RegisterGlobalFunction("Player@ GetPlayerByName(const string& in)", asFUNCTION(FindPlayerByName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("Creature@ GetCreatureByGuid(Unit@, uint64)", asFUNCTION(GetCreatureByGuid), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("GameObject@ GetGameObjectByGuid(Unit@, uint64)", asFUNCTION(GetGameObjectByGuid), asCALL_CDECL);

        // ---- Custom Hook Points ----
        // SendPlayerChoice hook: bool OnSendPlayerChoice(Player@, int32)
        r = _scriptEngine->RegisterGlobalFunction("void RegisterSendPlayerChoiceHook(int, ScriptCallback@)", asFUNCTION(RegisterCustomHook_SendPlayerChoice), asCALL_CDECL);
        // GetLockedDungeons hook: bool OnGetLockedDungeons(Player@)
        r = _scriptEngine->RegisterGlobalFunction("void RegisterGetLockedDungeonsHook(int, ScriptCallback@)", asFUNCTION(RegisterCustomHook_SendPlayerChoice), asCALL_CDECL);
        
        // ---- BattlePay Hook Funcdefs ----
        // AuthResponse hook: uint32 OnAuthResponse(WorldSession@ session, uint32 currencyID)
        // NOTE: After next recompile, change to WorldSession@ to allow session access in AS
        r = _scriptEngine->RegisterFuncdef("uint32 AuthResponseHook(uint32, uint32)");
        if (r >= 0 || r == asALREADY_REGISTERED)
            r = _scriptEngine->RegisterGlobalFunction("void RegisterAuthResponseHook(AuthResponseHook@)", asFUNCTION(RegisterCustomHook_AuthResponse), asCALL_CDECL);
        // FeatureSystemStatusGlueScreen hook: bool OnFeatureSystemStatusGlueScreen(WorldSession@ session, bool bpayStoreAvailable, int32 activeBoostType)
        // NOTE: After next recompile, change to WorldSession@ to allow session access in AS
        r = _scriptEngine->RegisterFuncdef("bool FeatureSystemStatusGlueScreenHook(uint32, bool, int32)");
        if (r >= 0 || r == asALREADY_REGISTERED)
            r = _scriptEngine->RegisterGlobalFunction("void RegisterFeatureSystemStatusGlueScreenHook(FeatureSystemStatusGlueScreenHook@)", asFUNCTION(RegisterCustomHook_FeatureSystemStatusGlueScreen), asCALL_CDECL);

        TC_LOG_INFO("server.angelscript", "World API registered (phase, summon, ObjectMgr, update fields, hook points)");
    }

} // namespace AngelScript
