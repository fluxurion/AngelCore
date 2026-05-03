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
#include <sstream>
#include <unordered_map>
#include <vector>

namespace AngelScript
{
    // Persistent storage for warband group names (WarbandGroup uses string_view)
    // Maps EnumCharactersResult* to a vector of strings that persist until cleared
    static std::unordered_map<void*, std::vector<std::string>> g_WarbandGroupNameStorage;

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
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "void AddMailSender(const string& in senderName, uint32 senderType)", asFUNCTION(CharInfo_AddMailSender), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("CharEnumCharacterInfo", "uint32 GetMailSenderCount()", asFUNCTION(CharInfo_GetMailSenderCount), asCALL_CDECL_OBJFIRST);

        // Register EnumCharactersResult type
        engine->RegisterObjectType("EnumCharactersResult", 0, asOBJ_REF | asOBJ_NOCOUNT);
        engine->RegisterObjectMethod("EnumCharactersResult", "uint32 GetCharacterCount()", asFUNCTION(EnumResult_GetCharacterCount), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "CharEnumCharacterInfo@ GetCharacter(uint32 index)", asFUNCTION(EnumResult_GetCharacter), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "CharEnumCharacterInfo@ FindCharacterByGuid(uint64 guidLow)", asFUNCTION(EnumResult_FindCharacterByGuid), asCALL_CDECL_OBJFIRST);

        // Warband group methods
        engine->RegisterObjectMethod("EnumCharactersResult", "void ClearWarbandGroups()", asFUNCTION(EnumResult_ClearWarbandGroups), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "uint32 GetWarbandGroupCount()", asFUNCTION(EnumResult_GetWarbandGroupCount), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void AddWarbandGroup(uint64 groupId, uint8 orderIndex, uint32 warbandSceneId, uint32 flags, int32 contentSetID, const string& in name)", asFUNCTION(EnumResult_AddWarbandGroup), asCALL_CDECL_OBJFIRST);
        engine->RegisterObjectMethod("EnumCharactersResult", "void AddWarbandGroupMember(uint32 groupIndex, uint32 slotIndex, int32 memberType, int32 contentSetID, uint64 guidLow)", asFUNCTION(EnumResult_AddWarbandGroupMember), asCALL_CDECL_OBJFIRST);
    }
}
