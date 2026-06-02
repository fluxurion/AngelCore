/*
 * AngelScript Character Packets API
 * Allows AngelScript to modify character enum data
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

#include "AngelScriptMgr.h"
#include "Server/Packets/CharacterPackets.h"
#include "Database/DatabaseEnv.h"
#include "World/World.h"
#include <sstream>
#include <unordered_map>
#include <deque>

namespace AngelScript
{
    // Persistent storage for warband group names (WarbandGroup uses string_view)
    // Maps EnumCharactersResult* to a deque of strings that persist until cleared.
    // A deque is used (not vector) so element addresses remain stable across
    // push_back — otherwise the string_view in WarbandGroup.Name would dangle
    // after the storage reallocates when a second group is added.
    static std::unordered_map<void*, std::deque<std::string>> g_WarbandGroupNameStorage;

    static void StoreWarbandGroupName(WorldPackets::Character::EnumCharactersResult* result, std::string&& name)
    {
        if (!result) return;
        auto& storage = g_WarbandGroupNameStorage[result];
        storage.push_back(std::move(name));
    }

    static void ClearWarbandGroupNameStorage(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return;
        g_WarbandGroupNameStorage.erase(result);
    }
    // ---- CharacterInfo helpers ----
    static uint64_t CharInfo_GetGuid(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return 0;
        return info->Basic.Guid.GetCounter();
    }

    static std::string CharInfo_GetName(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return "";
        return std::string(info->Basic.Name);
    }

    static void CharInfo_AddMailSender(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info, const std::string& senderName, uint32 senderType)
    {
        if (!info) return;
        info->RestrictionsAndMails.MailSenders.push_back(senderName);
        info->RestrictionsAndMails.MailSenderTypes.push_back(senderType);
    }

    static uint32 CharInfo_GetMailSenderCount(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return 0;
        return static_cast<uint32>(info->RestrictionsAndMails.MailSenders.size());
    }

    static uint8_t CharInfo_GetRace(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return 0;
        return info->Basic.RaceID;
    }

    static uint8_t CharInfo_GetClass(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return 0;
        return info->Basic.ClassID;
    }

    static uint8_t CharInfo_GetGender(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return 0;
        return info->Basic.SexID;
    }

    static uint8_t CharInfo_GetLevel(WorldPackets::Character::EnumCharactersResult::CharacterInfo* info)
    {
        if (!info) return 0;
        return info->Basic.ExperienceLevel;
    }

    // ---- RegionwideCharacterInfo helpers ----
    static uint64_t RegionwideInfo_GetGuid(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0;
        return info->Basic.Guid.GetCounter();
    }

    static std::string RegionwideInfo_GetName(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return "";
        return std::string(info->Basic.Name);
    }

    static uint64_t RegionwideInfo_GetMoney(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0;
        return info->Money;
    }

    static void RegionwideInfo_SetMoney(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, uint64_t money)
    {
        if (!info) return;
        info->Money = money;
    }

    static float RegionwideInfo_GetAvgItemLevel(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0.0f;
        return info->AvgEquippedItemLevel;
    }

    static void RegionwideInfo_SetAvgItemLevel(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, float itemLevel)
    {
        if (!info) return;
        info->AvgEquippedItemLevel = itemLevel;
    }

    static float RegionwideInfo_GetMythicPlusScore(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0.0f;
        return info->CurrentSeasonMythicPlusOverallScore;
    }

    static void RegionwideInfo_SetMythicPlusScore(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, float score)
    {
        if (!info) return;
        info->CurrentSeasonMythicPlusOverallScore = score;
    }

    static uint32_t RegionwideInfo_GetPvpRating(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0;
        return info->CurrentSeasonBestPvpRating;
    }

    static void RegionwideInfo_SetPvpRating(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, uint32_t rating)
    {
        if (!info) return;
        info->CurrentSeasonBestPvpRating = rating;
    }

    static int8_t RegionwideInfo_GetPvpBracket(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0;
        return info->PvpRatingBracket;
    }

    static void RegionwideInfo_SetPvpBracket(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, int8_t bracket)
    {
        if (!info) return;
        info->PvpRatingBracket = bracket;
    }

    static int16_t RegionwideInfo_GetPvpSpecId(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info)
    {
        if (!info) return 0;
        return info->PvpRatingAssociatedSpecID;
    }

    static void RegionwideInfo_SetPvpSpecId(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, int16_t specId)
    {
        if (!info) return;
        info->PvpRatingAssociatedSpecID = specId;
    }

    static int32 RegionwideInfo_GetProfessionId(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, uint32 index)
    {
        if (!info || index >= 2) return 0;
        return info->Basic.ProfessionIds[index];
    }

    static void RegionwideInfo_SetProfessionId(WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* info, uint32 index, int32 professionId)
    {
        if (!info || index >= 2) return;
        info->Basic.ProfessionIds[index] = professionId;
    }

    // ---- EnumCharactersResult helpers ----
    static uint32 EnumResult_GetCharacterCount(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return 0;
        return static_cast<uint32>(result->Characters.size());
    }

    static WorldPackets::Character::EnumCharactersResult::CharacterInfo* EnumResult_GetCharacter(WorldPackets::Character::EnumCharactersResult* result, uint32 index)
    {
        if (!result || index >= result->Characters.size()) return nullptr;
        return &result->Characters[index];
    }

    static WorldPackets::Character::EnumCharactersResult::CharacterInfo* EnumResult_FindCharacterByGuid(WorldPackets::Character::EnumCharactersResult* result, uint64_t guidLow)
    {
        if (!result) return nullptr;
        for (auto& character : result->Characters)
        {
            if (character.Basic.Guid.GetCounter() == guidLow)
                return &character;
        }
        return nullptr;
    }

    // ---- Warband Group helpers ----
    static bool EnumResult_IsDeletedCharacters(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return false;
        return result->IsDeletedCharacters;
    }

    static void EnumResult_ClearWarbandGroups(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return;
        result->WarbandGroups.clear();
        ClearWarbandGroupNameStorage(result);
    }

    static uint32_t EnumResult_GetWarbandGroupCount(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return 0;
        return static_cast<uint32>(result->WarbandGroups.size());
    }

    static void EnumResult_AddWarbandGroup(WorldPackets::Character::EnumCharactersResult* result, uint64 groupId, uint8 orderIndex,
                                           uint32 warbandSceneId, uint32 flags, int32 contentSetID, const std::string& name)
    {
        if (!result) return;

        // Store name in persistent storage (string_view needs stable memory)
        StoreWarbandGroupName(result, std::string(name));

        WorldPackets::Character::WarbandGroup group;
        group.GroupID = groupId;
        group.OrderIndex = orderIndex;
        group.WarbandSceneID = warbandSceneId;
        group.Flags = flags;
        group.ContentSetID = contentSetID;
        // Point string_view to our persistent storage
        group.Name = g_WarbandGroupNameStorage[result].back();
        result->WarbandGroups.push_back(std::move(group));
    }

    static void EnumResult_AddWarbandGroupMember(WorldPackets::Character::EnumCharactersResult* result, uint32 groupIndex,
                                                  uint32 warbandScenePlacementId, int32 memberType, int32 contentSetID, uint64 guidLow)
    {
        if (!result || groupIndex >= result->WarbandGroups.size()) return;
        WorldPackets::Character::WarbandGroupMember member;
        member.WarbandScenePlacementID = warbandScenePlacementId;
        member.Type = memberType;
        member.ContentSetID = contentSetID;
        member.Guid = ObjectGuid::Create<HighGuid::Player>(guidLow);
        result->WarbandGroups[groupIndex].Members.push_back(std::move(member));
    }

    // ---- RegionwideCharacter helpers ----
    static uint32_t EnumResult_GetRegionwideCharacterCount(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return 0;
        return static_cast<uint32>(result->RegionwideCharacters.size());
    }

    static WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* EnumResult_GetRegionwideCharacter(WorldPackets::Character::EnumCharactersResult* result, uint32 index)
    {
        if (!result || index >= result->RegionwideCharacters.size()) return nullptr;
        return &result->RegionwideCharacters[index];
    }

    static WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* EnumResult_FindRegionwideCharacterByGuid(WorldPackets::Character::EnumCharactersResult* result, uint64_t guidLow)
    {
        if (!result) return nullptr;
        for (auto& character : result->RegionwideCharacters)
        {
            if (character.Basic.Guid.GetCounter() == guidLow)
                return &character;
        }
        return nullptr;
    }

    static void EnumResult_ClearRegionwideCharacters(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return;
        result->RegionwideCharacters.clear();
    }

    static void EnumResult_ClearCharacters(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return;
        result->Characters.clear();
    }

    static WorldPackets::Character::EnumCharactersResult::RegionwideCharacterListEntry* EnumResult_AddRegionwideCharacter(
        WorldPackets::Character::EnumCharactersResult* result,
        uint64_t guidLow,
        const std::string& name,
        uint8_t raceID,
        uint8_t classID,
        uint8_t sexID,
        uint8_t level,
        uint64_t money,
        float itemLevel)
    {
        if (!result) return nullptr;

        // Create new entry directly in vector (RegionwideCharacterListEntry has no default ctor)
        result->RegionwideCharacters.emplace_back();
        auto& entry = result->RegionwideCharacters.back();

        // Set Basic info
        entry.Basic.Guid = ObjectGuid::Create<HighGuid::Player>(guidLow);
        // VirtualRealmAddress must match the current realm so the client associates
        // this regionwide entry with the realm and displays its data (money, ilvl, ...).
        // The default-constructed entry leaves this at 0, which the client rejects.
        entry.Basic.VirtualRealmAddress = GetVirtualRealmAddress();
        entry.Basic.Name = name;
        entry.Basic.RaceID = raceID;
        entry.Basic.ClassID = classID;
        entry.Basic.SexID = sexID;
        entry.Basic.ExperienceLevel = level;

        // Set Regionwide-specific data
        entry.Money = money;
        entry.AvgEquippedItemLevel = itemLevel;

        return &entry;
    }

    static void EnumResult_CopyCharactersToRegionwide(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return;

        result->RegionwideCharacters.clear();
        result->RegionwideCharacters.reserve(result->Characters.size());

        for (auto& charInfo : result->Characters)
        {
            result->RegionwideCharacters.emplace_back();
            auto& entry = result->RegionwideCharacters.back();

            // Copy the FULL CharacterInfoBasic (includes VisualItems, Customizations,
            // Flags, Guild info, MapID, ZoneID, PreloadPos, timestamps, etc.)
            entry.Basic = charInfo.Basic;

            // Initialize regionwide-specific fields (will be updated by AngelScript)
            entry.Money = 0;
            entry.AvgEquippedItemLevel = 0.0f;
            entry.CurrentSeasonMythicPlusOverallScore = 0.0f;
            entry.CurrentSeasonBestPvpRating = 0;
            entry.PvpRatingBracket = -1;
            entry.PvpRatingAssociatedSpecID = 0;
        }
    }

    static void EnumResult_SetRealmless(WorldPackets::Character::EnumCharactersResult* result, bool realmless)
    {
        if (!result) return;
        result->Realmless = realmless;
    }

    static bool EnumResult_GetRealmless(WorldPackets::Character::EnumCharactersResult* result)
    {
        if (!result) return false;
        return result->Realmless;
    }

    void CleanupWarbandGroupStorage(WorldPackets::Character::EnumCharactersResult* result)
    {
        ClearWarbandGroupNameStorage(result);
    }

    void RegisterCharacterPacketsAPI()
    {
        asIScriptEngine* engine = AngelScriptMgr::instance()->GetEngine();
        if (!engine) return;

        // Register CharacterInfo type
        engine->RegisterObjectType("CharEnumCharacterInfo", 0, asOBJ_REF | asOBJ_NOCOUNT);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint64 GetGuid()", asFUNCTION(CharInfo_GetGuid), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "string GetName()", asFUNCTION(CharInfo_GetName), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint8 GetRace()", asFUNCTION(CharInfo_GetRace), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint8 GetClass()", asFUNCTION(CharInfo_GetClass), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint8 GetGender()", asFUNCTION(CharInfo_GetGender), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint8 GetLevel()", asFUNCTION(CharInfo_GetLevel), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "void AddMailSender(const string& in senderName, uint32 senderType)", asFUNCTION(CharInfo_AddMailSender), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint32 GetMailSenderCount()", asFUNCTION(CharInfo_GetMailSenderCount), asCALL_CDECL_OBJFIRST);

        // Register EnumCharactersResult type
        engine->RegisterObjectType("EnumCharactersResult", 0, asOBJ_REF | asOBJ_NOCOUNT);
        engine->RegisterObjectMethod("EnumCharactersResult", "uint32 GetCharacterCount()", asFUNCTION(EnumResult_GetCharacterCount), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "CharEnumCharacterInfo@ GetCharacter(uint32 index)", asFUNCTION(EnumResult_GetCharacter), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "CharEnumCharacterInfo@ FindCharacterByGuid(uint64 guidLow)", asFUNCTION(EnumResult_FindCharacterByGuid), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void ClearCharacters()", asFUNCTION(EnumResult_ClearCharacters), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void CopyCharactersToRegionwide()", asFUNCTION(EnumResult_CopyCharactersToRegionwide), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void SetRealmless(bool realmless)", asFUNCTION(EnumResult_SetRealmless), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "bool GetRealmless()", asFUNCTION(EnumResult_GetRealmless), asCALL_CDECL_OBJFIRST);

        // Warband group methods
        engine->RegisterObjectMethod("EnumCharactersResult", "bool IsDeletedCharacters()", asFUNCTION(EnumResult_IsDeletedCharacters), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void ClearWarbandGroups()", asFUNCTION(EnumResult_ClearWarbandGroups), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "uint32 GetWarbandGroupCount()", asFUNCTION(EnumResult_GetWarbandGroupCount), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void AddWarbandGroup(uint64 groupId, uint8 orderIndex, uint32 warbandSceneId, uint32 flags, int32 contentSetID, const string& in name)", asFUNCTION(EnumResult_AddWarbandGroup), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void AddWarbandGroupMember(uint32 groupIndex, uint32 slotIndex, int32 memberType, int32 contentSetID, uint64 guidLow)", asFUNCTION(EnumResult_AddWarbandGroupMember), asCALL_CDECL_OBJFIRST);

        // Register RegionwideCharacterInfo type
        engine->RegisterObjectType("RegionwideCharacterInfo", 0, asOBJ_REF | asOBJ_NOCOUNT);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "uint64 GetGuid()", asFUNCTION(RegionwideInfo_GetGuid), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "string GetName()", asFUNCTION(RegionwideInfo_GetName), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "uint64 GetMoney()", asFUNCTION(RegionwideInfo_GetMoney), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetMoney(uint64 money)", asFUNCTION(RegionwideInfo_SetMoney), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "float GetAvgItemLevel()", asFUNCTION(RegionwideInfo_GetAvgItemLevel), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetAvgItemLevel(float itemLevel)", asFUNCTION(RegionwideInfo_SetAvgItemLevel), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "float GetMythicPlusScore()", asFUNCTION(RegionwideInfo_GetMythicPlusScore), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetMythicPlusScore(float score)", asFUNCTION(RegionwideInfo_SetMythicPlusScore), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "uint32 GetPvpRating()", asFUNCTION(RegionwideInfo_GetPvpRating), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetPvpRating(uint32 rating)", asFUNCTION(RegionwideInfo_SetPvpRating), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "int8 GetPvpBracket()", asFUNCTION(RegionwideInfo_GetPvpBracket), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetPvpBracket(int8 bracket)", asFUNCTION(RegionwideInfo_SetPvpBracket), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "int16 GetPvpSpecId()", asFUNCTION(RegionwideInfo_GetPvpSpecId), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetPvpSpecId(int16 specId)", asFUNCTION(RegionwideInfo_SetPvpSpecId), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "int32 GetProfessionId(uint32 index)", asFUNCTION(RegionwideInfo_GetProfessionId), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("RegionwideCharacterInfo", "void SetProfessionId(uint32 index, int32 professionId)", asFUNCTION(RegionwideInfo_SetProfessionId), asCALL_CDECL_OBJFIRST);

        // RegionwideCharacter methods on EnumCharactersResult
        engine->RegisterObjectMethod("EnumCharactersResult", "uint32 GetRegionwideCharacterCount()", asFUNCTION(EnumResult_GetRegionwideCharacterCount), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "RegionwideCharacterInfo@ GetRegionwideCharacter(uint32 index)", asFUNCTION(EnumResult_GetRegionwideCharacter), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "RegionwideCharacterInfo@ FindRegionwideCharacterByGuid(uint64 guidLow)", asFUNCTION(EnumResult_FindRegionwideCharacterByGuid), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void ClearRegionwideCharacters()", asFUNCTION(EnumResult_ClearRegionwideCharacters), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "RegionwideCharacterInfo@ AddRegionwideCharacter(uint64 guidLow, const string& in name, uint8 raceID, uint8 classID, uint8 sexID, uint8 level, uint64 money, float itemLevel)", asFUNCTION(EnumResult_AddRegionwideCharacter), asCALL_CDECL_OBJFIRST);
    }
}
