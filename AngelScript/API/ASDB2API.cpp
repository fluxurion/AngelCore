/*
 * AngelScript DB2 Data API
 * Exposes TrinityCore's already-loaded DB2 stores to AngelScript.
 * TC loads all DB2 data at server startup — we just share the pointers.
 *
 * Design:
 * - DB2Entry is a generic handle to any DB2 record (by store hash + record ID)
 * - High-level accessor functions for the most commonly used stores
 * - Generic query functions for any store by name
 * - Scripts can iterate stores and read fields by index
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

#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DB2Store.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "../ASDB2Schema.h"
#include "../ASDB2Storage.h"
#include <string>
#include <memory>
#include <scriptarray/scriptarray.h>

namespace AngelScript
{
    // ========================================================================
    // DB2Entry — lightweight handle to a DB2 record
    // ========================================================================

    struct DB2Entry
    {
        uint32 ID = 0;
        // Store which store this came from for type safety
        // But we keep it simple — just store the entry pointer
    };

    // ========================================================================
    // Spell DB2 accessors (most commonly needed)
    // ========================================================================

    static std::string DB2_GetSpellName(uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!info) return "";
        // SpellName is LocalizedString const* - need to dereference
        if (!info->SpellName) return "";
        const char* spellName = (*info->SpellName)[static_cast<LocaleConstant>(0)];
        return spellName ? spellName : "";
    }

    static uint32 DB2_GetSpellSchool(uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        return info ? info->SchoolMask : 0;
    }

    static int32 DB2_GetSpellCastTime(uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        return (info && info->CastTimeEntry) ? info->CastTimeEntry->Base : 0;
    }

    static int32 DB2_GetSpellDuration(uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        return (info && info->DurationEntry) ? info->DurationEntry->Duration : 0;
    }

    static uint32 DB2_GetSpellRange(uint32 spellId)
    {
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        return (info && info->RangeEntry) ? info->RangeEntry->ID : 0;
    }

    static bool DB2_HasSpell(uint32 spellId)
    {
        return sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE) != nullptr;
    }

    // ========================================================================
    // Item DB2 accessors
    // ========================================================================

    static std::string DB2_GetItemName(uint32 itemId)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        return proto ? proto->GetName(static_cast<LocaleConstant>(0)) : "";
    }

    static uint32 DB2_GetItemClass(uint32 itemId)
    {
        ItemEntry const* entry = sItemStore.LookupEntry(itemId);
        return entry ? static_cast<uint32>(entry->ClassID) : 0;
    }

    static uint32 DB2_GetItemSubClass(uint32 itemId)
    {
        ItemEntry const* entry = sItemStore.LookupEntry(itemId);
        return entry ? static_cast<uint32>(entry->SubclassID) : 0;
    }

    static int32 DB2_GetItemInventoryType(uint32 itemId)
    {
        ItemEntry const* entry = sItemStore.LookupEntry(itemId);
        return entry ? static_cast<int32>(entry->InventoryType) : -1;
    }

    static bool DB2_HasItem(uint32 itemId)
    {
        return sItemStore.LookupEntry(itemId) != nullptr;
    }

    // ========================================================================
    // Creature DB2 accessors
    // ========================================================================

    static float DB2_GetCreatureDisplayScale(uint32 displayId)
    {
        CreatureDisplayInfoEntry const* entry = sCreatureDisplayInfoStore.LookupEntry(displayId);
        return entry ? entry->CreatureModelScale : 1.0f;
    }

    static uint32 DB2_GetCreatureDisplayModel(uint32 displayId)
    {
        CreatureDisplayInfoEntry const* entry = sCreatureDisplayInfoStore.LookupEntry(displayId);
        return entry ? static_cast<uint32>(entry->ModelID) : 0;
    }

    static bool DB2_HasCreatureDisplay(uint32 displayId)
    {
        return sCreatureDisplayInfoStore.LookupEntry(displayId) != nullptr;
    }

    // ========================================================================
    // Map DB2 accessors
    // ========================================================================

    static std::string DB2_GetMapName(uint32 mapId)
    {
        MapEntry const* entry = sMapStore.LookupEntry(mapId);
        if (!entry) return "";
        return entry->MapName[static_cast<LocaleConstant>(0)]; // default locale
    }

    static uint32 DB2_GetMapAreaId(uint32 mapId)
    {
        MapEntry const* entry = sMapStore.LookupEntry(mapId);
        return entry ? entry->AreaTableID : 0;
    }

    static bool DB2_IsDungeon(uint32 mapId)
    {
        MapEntry const* entry = sMapStore.LookupEntry(mapId);
        return entry ? entry->IsDungeon() : false;
    }

    static bool DB2_IsRaid(uint32 mapId)
    {
        MapEntry const* entry = sMapStore.LookupEntry(mapId);
        return entry ? entry->IsRaid() : false;
    }

    static bool DB2_IsBattleground(uint32 mapId)
    {
        MapEntry const* entry = sMapStore.LookupEntry(mapId);
        return entry ? entry->IsBattleground() : false;
    }

    // ========================================================================
    // Area DB2 accessors
    // ========================================================================

    static std::string DB2_GetAreaName(uint32 areaId)
    {
        AreaTableEntry const* entry = sAreaTableStore.LookupEntry(areaId);
        if (!entry) return "";
        return entry->AreaName[static_cast<LocaleConstant>(0)];
    }

    static uint32 DB2_GetAreaParentAreaId(uint32 areaId)
    {
        AreaTableEntry const* entry = sAreaTableStore.LookupEntry(areaId);
        return entry ? entry->ParentAreaID : 0;
    }

    static uint32 DB2_GetAreaMapId(uint32 areaId)
    {
        AreaTableEntry const* entry = sAreaTableStore.LookupEntry(areaId);
        return entry ? entry->ContinentID : 0;
    }

    // ========================================================================
    // Faction DB2 accessors
    // ========================================================================

    static std::string DB2_GetFactionName(uint32 factionId)
    {
        FactionEntry const* entry = sFactionStore.LookupEntry(factionId);
        if (!entry) return "";
        return entry->Name[static_cast<LocaleConstant>(0)];
    }

    static uint32 DB2_GetFactionTemplateFaction(uint32 factionTemplateId)
    {
        FactionTemplateEntry const* entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
        return entry ? entry->Faction : 0; // Faction is uint32, not LocalizedString
    }

    static uint32 DB2_GetFactionTemplateFlags(uint32 factionTemplateId)
    {
        FactionTemplateEntry const* entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
        return entry ? entry->Flags : 0;
    }

    // ========================================================================
    // Class/Race DB2 accessors
    // ========================================================================

    static std::string DB2_GetClassName(uint32 classId)
    {
        ChrClassesEntry const* entry = sChrClassesStore.LookupEntry(classId);
        if (!entry) return "";
        return entry->Name[static_cast<LocaleConstant>(0)];
    }

    static std::string DB2_GetRaceName(uint32 raceId)
    {
        ChrRacesEntry const* entry = sChrRacesStore.LookupEntry(raceId);
        if (!entry) return "";
        return entry->Name[static_cast<LocaleConstant>(0)];
    }

    static uint32 DB2_GetRaceFactionId(uint32 /*raceId*/)
    {
        // ChrRacesEntry doesn't have a Faction field in this TC version
        return 0;
    }

    // ========================================================================
    // Skill DB2 accessors
    // ========================================================================

    static std::string DB2_GetSkillName(uint32 skillId)
    {
        SkillLineEntry const* entry = sSkillLineStore.LookupEntry(skillId);
        if (!entry) return "";
        return entry->DisplayName[static_cast<LocaleConstant>(0)];
    }

    // ========================================================================
    // Emote DB2 accessors
    // ========================================================================

    static int32 DB2_GetEmoteAnimId(uint32 emoteId)
    {
        EmotesEntry const* entry = sEmotesStore.LookupEntry(emoteId);
        return entry ? entry->AnimID : 0;
    }

    // ========================================================================
    // Difficulty DB2 accessors
    // ========================================================================

    static std::string DB2_GetDifficultyName(uint32 difficultyId)
    {
        DifficultyEntry const* entry = sDifficultyStore.LookupEntry(difficultyId);
        if (!entry) return "";
        return entry->Name[static_cast<LocaleConstant>(0)];
    }

    // ========================================================================
    // Generic DB2 store queries
    // ========================================================================

    static uint32 DB2_GetStoreEntryCount(const std::string& storeName)
    {
        // Map store names to their storage
        if (storeName == "SpellName") return sSpellNameStore.GetNumRows();
        if (storeName == "Item") return sItemStore.GetNumRows();
        if (storeName == "CreatureDisplayInfo") return sCreatureDisplayInfoStore.GetNumRows();
        if (storeName == "Map") return sMapStore.GetNumRows();
        if (storeName == "AreaTable") return sAreaTableStore.GetNumRows();
        if (storeName == "Faction") return sFactionStore.GetNumRows();
        if (storeName == "FactionTemplate") return sFactionTemplateStore.GetNumRows();
        if (storeName == "ChrClasses") return sChrClassesStore.GetNumRows();
        if (storeName == "ChrRaces") return sChrRacesStore.GetNumRows();
        if (storeName == "SkillLine") return sSkillLineStore.GetNumRows();
        if (storeName == "Emotes") return sEmotesStore.GetNumRows();
        if (storeName == "Difficulty") return sDifficultyStore.GetNumRows();
        if (storeName == "CharTitles") return sCharTitlesStore.GetNumRows();
        if (storeName == "CurrencyTypes") return sCurrencyTypesStore.GetNumRows();
        if (storeName == "GameObjectDisplayInfo") return sGameObjectDisplayInfoStore.GetNumRows();
        if (storeName == "SoundKit") return sSoundKitStore.GetNumRows();
        if (storeName == "SpellCategory") return sSpellCategoryStore.GetNumRows();
        if (storeName == "SpellCastTimes") return sSpellCastTimesStore.GetNumRows();
        if (storeName == "SpellDuration") return sSpellDurationStore.GetNumRows();
        if (storeName == "SpellRange") return sSpellRangeStore.GetNumRows();
        if (storeName == "SpellEffect") return sSpellEffectStore.GetNumRows();
        if (storeName == "SpellVisual") return sSpellVisualStore.GetNumRows();
        if (storeName == "BroadcastText") return sBroadcastTextStore.GetNumRows();
        if (storeName == "QuestSort") return sQuestSortStore.GetNumRows();
        if (storeName == "QuestInfo") return sQuestInfoStore.GetNumRows();
        if (storeName == "TaxiNodes") return sTaxiNodesStore.GetNumRows();
        if (storeName == "TaxiPath") return sTaxiPathStore.GetNumRows();
        if (storeName == "Achievement") return sAchievementStore.GetNumRows();
        TC_LOG_WARN("angelscript", "Unknown DB2 store: {}", storeName);
        return 0;
    }

    static bool DB2_HasStoreEntry(const std::string& storeName, uint32 id)
    {
        if (storeName == "SpellName") return sSpellNameStore.LookupEntry(id) != nullptr;
        if (storeName == "Item") return sItemStore.LookupEntry(id) != nullptr;
        if (storeName == "CreatureDisplayInfo") return sCreatureDisplayInfoStore.LookupEntry(id) != nullptr;
        if (storeName == "Map") return sMapStore.LookupEntry(id) != nullptr;
        if (storeName == "AreaTable") return sAreaTableStore.LookupEntry(id) != nullptr;
        if (storeName == "Faction") return sFactionStore.LookupEntry(id) != nullptr;
        if (storeName == "FactionTemplate") return sFactionTemplateStore.LookupEntry(id) != nullptr;
        if (storeName == "ChrClasses") return sChrClassesStore.LookupEntry(id) != nullptr;
        if (storeName == "ChrRaces") return sChrRacesStore.LookupEntry(id) != nullptr;
        if (storeName == "SkillLine") return sSkillLineStore.LookupEntry(id) != nullptr;
        if (storeName == "Emotes") return sEmotesStore.LookupEntry(id) != nullptr;
        if (storeName == "Difficulty") return sDifficultyStore.LookupEntry(id) != nullptr;
        return false;
    }

    // ========================================================================
    // DB2Manager function exposure
    // ========================================================================

    static float DB2_GetCurveValue(uint32 curveId, float x)
    {
        return sDB2Manager.GetCurveValueAt(curveId, x);
    }

    static std::string DB2_GetBroadcastText(uint32 id)
    {
        BroadcastTextEntry const* entry = sBroadcastTextStore.LookupEntry(id);
        if (!entry) return "";
        return DB2Manager::GetBroadcastTextValue(entry);
    }

    static std::string DB2_GetCreatureFamilyPetName(uint32 familyId)
    {
        char const* name = DB2Manager::GetCreatureFamilyPetName(familyId, LOCALE_enUS);
        return name ? std::string(name) : "";
    }

    static bool DB2_IsToyItem(uint32 itemId)
    {
        return sDB2Manager.IsToyItem(itemId);
    }

    static uint32 DB2_GetGlobalCurveId(uint32 globalCurveType)
    {
        return sDB2Manager.GetGlobalCurveId(static_cast<GlobalCurve>(globalCurveType));
    }

    // ========================================================================
    // Title DB2 accessors
    // ========================================================================

    static std::string DB2_GetCharTitleName(uint32 titleId)
    {
        CharTitlesEntry const* entry = sCharTitlesStore.LookupEntry(titleId);
        if (!entry) return "";
        return entry->Name[static_cast<LocaleConstant>(0)];
    }

    // ========================================================================
    // Currency DB2 accessors
    // ========================================================================

    static std::string DB2_GetCurrencyName(uint32 currencyId)
    {
        CurrencyTypesEntry const* entry = sCurrencyTypesStore.LookupEntry(currencyId);
        if (!entry) return "";
        return entry->Name[static_cast<LocaleConstant>(0)];
    }

    // ========================================================================
    // Registration
    // ========================================================================

    void RegisterDB2API(asIScriptEngine* _scriptEngine)
    {
        // ---- Spell DB2 ----
        int r;
        r = _scriptEngine->RegisterGlobalFunction("string GetSpellName(uint32)", asFUNCTION(DB2_GetSpellName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetSpellSchool(uint32)", asFUNCTION(DB2_GetSpellSchool), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("int32 GetSpellCastTimeDB(uint32)", asFUNCTION(DB2_GetSpellCastTime), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("int32 GetSpellDurationDB(uint32)", asFUNCTION(DB2_GetSpellDuration), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetSpellRangeDB(uint32)", asFUNCTION(DB2_GetSpellRange), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasSpellDB(uint32)", asFUNCTION(DB2_HasSpell), asCALL_CDECL);

        // ---- Item DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetItemName(uint32)", asFUNCTION(DB2_GetItemName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetItemClass(uint32)", asFUNCTION(DB2_GetItemClass), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetItemSubClass(uint32)", asFUNCTION(DB2_GetItemSubClass), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("int32 GetItemInventoryType(uint32)", asFUNCTION(DB2_GetItemInventoryType), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasItemDB(uint32)", asFUNCTION(DB2_HasItem), asCALL_CDECL);

        // ---- Creature DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("float GetCreatureDisplayScale(uint32)", asFUNCTION(DB2_GetCreatureDisplayScale), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetCreatureDisplayModel(uint32)", asFUNCTION(DB2_GetCreatureDisplayModel), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasCreatureDisplayDB(uint32)", asFUNCTION(DB2_HasCreatureDisplay), asCALL_CDECL);

        // ---- Map DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetMapName(uint32)", asFUNCTION(DB2_GetMapName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetMapAreaId(uint32)", asFUNCTION(DB2_GetMapAreaId), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool IsDungeonMap(uint32)", asFUNCTION(DB2_IsDungeon), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool IsRaidMap(uint32)", asFUNCTION(DB2_IsRaid), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool IsBattlegroundMap(uint32)", asFUNCTION(DB2_IsBattleground), asCALL_CDECL);

        // ---- Area DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetAreaName(uint32)", asFUNCTION(DB2_GetAreaName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetAreaParentAreaId(uint32)", asFUNCTION(DB2_GetAreaParentAreaId), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetAreaMapId(uint32)", asFUNCTION(DB2_GetAreaMapId), asCALL_CDECL);

        // ---- Faction DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetFactionName(uint32)", asFUNCTION(DB2_GetFactionName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetFactionTemplateFaction(uint32)", asFUNCTION(DB2_GetFactionTemplateFaction), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetFactionTemplateFlags(uint32)", asFUNCTION(DB2_GetFactionTemplateFlags), asCALL_CDECL);

        // ---- Class/Race DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetClassName(uint32)", asFUNCTION(DB2_GetClassName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetRaceName(uint32)", asFUNCTION(DB2_GetRaceName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetRaceFactionId(uint32)", asFUNCTION(DB2_GetRaceFactionId), asCALL_CDECL);

        // ---- Skill DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetSkillName(uint32)", asFUNCTION(DB2_GetSkillName), asCALL_CDECL);

        // ---- Emote DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("int32 GetEmoteAnimId(uint32)", asFUNCTION(DB2_GetEmoteAnimId), asCALL_CDECL);

        // ---- Difficulty DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetDifficultyName(uint32)", asFUNCTION(DB2_GetDifficultyName), asCALL_CDECL);

        // ---- Title DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetCharTitleName(uint32)", asFUNCTION(DB2_GetCharTitleName), asCALL_CDECL);

        // ---- Currency DB2 ----
        r = _scriptEngine->RegisterGlobalFunction("string GetCurrencyName(uint32)", asFUNCTION(DB2_GetCurrencyName), asCALL_CDECL);

        // ---- Generic DB2 store queries ----
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetDB2StoreEntryCount(const string& in)", asFUNCTION(DB2_GetStoreEntryCount), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool HasDB2StoreEntry(const string& in, uint32)", asFUNCTION(DB2_HasStoreEntry), asCALL_CDECL);

        // ---- DB2Manager functions ----
        r = _scriptEngine->RegisterGlobalFunction("float GetCurveValue(uint32, float)", asFUNCTION(DB2_GetCurveValue), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetBroadcastText(uint32)", asFUNCTION(DB2_GetBroadcastText), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetCreatureFamilyPetName(uint32)", asFUNCTION(DB2_GetCreatureFamilyPetName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool IsToyItem(uint32)", asFUNCTION(DB2_IsToyItem), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetGlobalCurveId(uint32)", asFUNCTION(DB2_GetGlobalCurveId), asCALL_CDECL);

        TC_LOG_INFO("server.angelscript", "DB2 API registered (shared TC stores)");
    }

    // ========================================================================
    // NEW: Dynamic DB2 Loading API
    // Allows AngelScript to load custom DB2 files with runtime-defined schemas
    // ========================================================================

    // Schema creation
    static ASDB2Schema* DB2_CreateSchema(const std::string& name)
    {
        auto schema = std::make_shared<ASDB2Schema>(name);
        sASDB2SchemaRegistry->RegisterSchema(schema);
        // Return raw pointer - AngelScript will use reference counting
        return schema.get();
    }

    static void DB2_SchemaAddInt8(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddInt8(name, count); }
    static void DB2_SchemaAddUInt8(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddUInt8(name, count); }
    static void DB2_SchemaAddInt16(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddInt16(name, count); }
    static void DB2_SchemaAddUInt16(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddUInt16(name, count); }
    static void DB2_SchemaAddInt32(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddInt32(name, count); }
    static void DB2_SchemaAddUInt32(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddUInt32(name, count); }
    static void DB2_SchemaAddInt64(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddInt64(name, count); }
    static void DB2_SchemaAddUInt64(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddUInt64(name, count); }
    static void DB2_SchemaAddFloat(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddFloat(name, count); }
    static void DB2_SchemaAddDouble(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddDouble(name, count); }
    static void DB2_SchemaAddString(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddString(name, count); }
    static void DB2_SchemaAddBool(ASDB2Schema* schema, const std::string& name, uint32 count) { schema->AddBool(name, count); }

    static void DB2_SchemaFinalize(ASDB2Schema* schema)
    {
        if (schema) schema->Finalize();
    }

    // Storage loading
    static uint32 DB2_LoadStorage(const std::string& name, const std::string& filePath, ASDB2Schema* schema)
    {
        if (!schema)
        {
            TC_LOG_ERROR("angelscript.db2", "Cannot load storage '{}': null schema", name);
            return 0;
        }

        // Share ownership with registry
        auto schemaPtr = std::make_shared<ASDB2Schema>(schema->GetName());
        *schemaPtr = *schema; // Copy schema data
        
        return sASDB2StorageRegistry->CreateStorage(name, filePath, schemaPtr);
    }

    static ASDB2Storage* DB2_GetStorage(const std::string& name)
    {
        return sASDB2StorageRegistry->GetStorage(name);
    }

    static ASDB2Storage* DB2_GetStorageByHandle(uint32 handle)
    {
        return sASDB2StorageRegistry->GetStorage(handle);
    }

    static void DB2_UnloadStorage(const std::string& name)
    {
        sASDB2StorageRegistry->RemoveStorage(name);
    }

    static void DB2_UnloadStorageByHandle(uint32 handle)
    {
        sASDB2StorageRegistry->RemoveStorage(handle);
    }

    // Record access
    static bool DB2_StorageHasRecord(ASDB2Storage* storage, uint32 id)
    {
        return storage ? storage->HasRecord(id) : false;
    }

    static ASDB2DynamicRecord* DB2_StorageGetRecord(ASDB2Storage* storage, uint32 id)
    {
        if (!storage) return nullptr;
        // Return non-const pointer for AngelScript
        return const_cast<ASDB2DynamicRecord*>(storage->GetRecord(id));
    }

    static uint32 DB2_StorageGetNumRows(ASDB2Storage* storage)
    {
        return storage ? storage->GetNumRows() : 0;
    }

    static CScriptArray* DB2_StorageGetAllIds(ASDB2Storage* storage)
    {
        if (!storage) return nullptr;

        asIScriptContext* ctx = asGetActiveContext();
        if (!ctx) return nullptr;
        asIScriptEngine* engine = ctx->GetEngine();
        asITypeInfo* arrayType = engine->GetTypeInfoByDecl("array<uint32>");
        
        CScriptArray* array = CScriptArray::Create(arrayType);
        std::vector<uint32> ids = storage->GetAllIds();
        array->Resize(static_cast<uint32>(ids.size()));
        
        for (uint32 i = 0; i < ids.size(); ++i)
        {
            *(uint32*)array->At(i) = ids[i];
        }
        
        return array;
    }

    // Record field access
    static uint32 DB2_RecordGetUInt32(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetUInt32(fieldName) : 0;
    }

    static int32 DB2_RecordGetInt32(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetInt32(fieldName) : 0;
    }

    static float DB2_RecordGetFloat(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetFloat(fieldName) : 0.0f;
    }

    static uint64 DB2_RecordGetUInt64(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetUInt64(fieldName) : 0;
    }

    static int64 DB2_RecordGetInt64(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetInt64(fieldName) : 0;
    }

    static std::string DB2_RecordGetString(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetString(fieldName) : "";
    }

    static bool DB2_RecordGetBool(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->GetBool(fieldName) : false;
    }

    static bool DB2_RecordHasField(ASDB2DynamicRecord* record, const std::string& fieldName)
    {
        return record ? record->HasField(fieldName) : false;
    }

    static uint32 DB2_RecordGetFieldCount(ASDB2DynamicRecord* record)
    {
        return record ? record->GetFieldCount() : 0;
    }

    void RegisterDynamicDB2API(asIScriptEngine* engine)
    {
        int r;

        // ---- DB2Schema class ----
        r = engine->RegisterObjectType("DB2Schema", 0, asOBJ_REF | asOBJ_NOCOUNT);
        
        // Schema factory
        r = engine->RegisterGlobalFunction("DB2Schema@ CreateDB2Schema(const string& in)", 
            asFUNCTION(DB2_CreateSchema), asCALL_CDECL);

        // Schema field addition
        r = engine->RegisterObjectMethod("DB2Schema", "void AddInt8(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddInt8), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddUInt8(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddUInt8), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddInt16(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddInt16), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddUInt16(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddUInt16), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddInt32(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddInt32), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddUInt32(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddUInt32), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddInt64(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddInt64), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddUInt64(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddUInt64), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddFloat(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddFloat), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddDouble(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddDouble), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddString(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddString), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Schema", "void AddBool(const string& in, uint32 count = 1)", asFUNCTION(DB2_SchemaAddBool), asCALL_CDECL_OBJFIRST);

        r = engine->RegisterObjectMethod("DB2Schema", "void Finalize()", 
            asFUNCTION(DB2_SchemaFinalize), asCALL_CDECL_OBJFIRST);

        // ---- DB2Record class (forward-declared before DB2Storage so GetRecord can reference it) ----
        r = engine->RegisterObjectType("DB2Record", 0, asOBJ_REF | asOBJ_NOCOUNT);

        // ---- DB2Storage class ----
        r = engine->RegisterObjectType("DB2Storage", 0, asOBJ_REF | asOBJ_NOCOUNT);

        // Storage loading/management
        r = engine->RegisterGlobalFunction("uint32 LoadDB2Storage(const string& in, const string& in, DB2Schema@)", 
            asFUNCTION(DB2_LoadStorage), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("DB2Storage@ GetDB2Storage(const string& in)", 
            asFUNCTION(DB2_GetStorage), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("DB2Storage@ GetDB2StorageByHandle(uint32)", 
            asFUNCTION(DB2_GetStorageByHandle), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void UnloadDB2Storage(const string& in)", 
            asFUNCTION(DB2_UnloadStorage), asCALL_CDECL);
        r = engine->RegisterGlobalFunction("void UnloadDB2StorageByHandle(uint32)", 
            asFUNCTION(DB2_UnloadStorageByHandle), asCALL_CDECL);

        // Storage methods
        r = engine->RegisterObjectMethod("DB2Storage", "bool HasRecord(uint32)", 
            asFUNCTION(DB2_StorageHasRecord), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Storage", "DB2Record@ GetRecord(uint32)", 
            asFUNCTION(DB2_StorageGetRecord), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Storage", "uint32 GetNumRows()", 
            asFUNCTION(DB2_StorageGetNumRows), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Storage", "array<uint32>@ GetAllIds()", 
            asFUNCTION(DB2_StorageGetAllIds), asCALL_CDECL_OBJFIRST);

        // ---- DB2Record methods (type already registered above) ----
        r = engine->RegisterObjectMethod("DB2Record", "uint32 GetUInt32(const string& in)", 
            asFUNCTION(DB2_RecordGetUInt32), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "int32 GetInt32(const string& in)", 
            asFUNCTION(DB2_RecordGetInt32), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "float GetFloat(const string& in)", 
            asFUNCTION(DB2_RecordGetFloat), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "uint64 GetUInt64(const string& in)", 
            asFUNCTION(DB2_RecordGetUInt64), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "int64 GetInt64(const string& in)", 
            asFUNCTION(DB2_RecordGetInt64), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "string GetString(const string& in)", 
            asFUNCTION(DB2_RecordGetString), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "bool GetBool(const string& in)", 
            asFUNCTION(DB2_RecordGetBool), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "bool HasField(const string& in)", 
            asFUNCTION(DB2_RecordHasField), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("DB2Record", "uint32 GetFieldCount()", 
            asFUNCTION(DB2_RecordGetFieldCount), asCALL_CDECL_OBJFIRST);

        TC_LOG_INFO("server.angelscript", "Dynamic DB2 API registered");
    }

} // namespace AngelScript

