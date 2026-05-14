/*
 * AngelScript Global Functions & World/Math/String API
 * Print, SendSystemMessage, SendFloatingText, PlaySound, World accessors, etc.
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
#include "../SDK/add_on/scriptarray/scriptarray.h"

#pragma pop_macro("OPTIONAL")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")

#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "World.h"
#include "WorldSession.h"
#include <cmath>
#include "ObjectAccessor.h"
#include "ChatTextBuilder.h"
#include "Chat.h"
#include "GameTime.h"
#include "Log.h"
#include <string>
#include "SpellDefines.h"
#include "Config.h"
#include "AngelScriptMgr.h"

namespace AngelScript
{
    // Math constants (must be defined before use)
    static float Math_PI = 3.14159265358979323846f;
    static float Math_Half_PI = 1.57079632679489661923f;

    // ---- Global utility functions ----

    static void Global_Print(const std::string& msg)
    {
        TC_LOG_INFO("server.angelscript", "{}", msg);
    }

    static void Global_SendSystemMessage(Player* player, const std::string& msg)
    {
        if (player && player->GetSession())
            ChatHandler(player->GetSession()).PSendSysMessage("%s", msg.c_str());
    }

    static void Global_SendFloatingText(Player* player, const std::string& text, uint32 /*color*/)
    {
        if (!player || !player->GetSession()) return;
        // Use WorldPacket to send floating combat text
        // This is a simplified implementation using SendNotification as fallback
        player->GetSession()->SendNotification("%s", text.c_str());
    }

    static void Global_PlaySoundToPlayer(Player* player, uint32 soundId)
    {
        if (!player) return;
        player->PlayDirectSound(soundId, player);
    }

    static void Global_SendWorldText(const std::string& msg)
    {
        sWorld->SendServerMessage(SERVER_MSG_STRING, msg);
    }

    static void Global_SendWorldTextTo(uint32 /*mapId*/, const std::string& msg)
    {
        sWorld->SendServerMessage(SERVER_MSG_STRING, msg);
    }

    // ---- World API wrappers ----
    static uint32 World_GetTime() { return static_cast<uint32>(GameTime::GetGameTime()); }
    static uint32 World_GetGameTime() { return static_cast<uint32>(GameTime::GetGameTime()); }
    static uint32 World_GetMSTime() { return getMSTime(); }
    static uint32 World_GetConfigUInt32(uint32 configId) { return sWorld->getIntConfig(static_cast<WorldIntConfigs>(configId)); }
    static float World_GetConfigFloat(uint32 configId) { return sWorld->getFloatConfig(static_cast<WorldFloatConfigs>(configId)); }
    static bool World_GetConfigBool(uint32 configId) { return sWorld->getBoolConfig(static_cast<WorldBoolConfigs>(configId)); }
    static uint32 World_GetPlayerCount() { return sWorld->GetActiveSessionCount(); }
    static uint32 World_GetMaxPlayerCount() { return sWorld->GetMaxActiveSessionCount(); }

    // ---- CastSpell with custom base points ----
    static void Global_CastSpellWithBP(Unit* caster, Unit* target, uint32 spellId, float bp0, float bp1)
    {
        if (!caster || !target) return;
        CastSpellExtraArgs args(TRIGGERED_FULL_MASK);
        args.AddSpellMod(SPELLVALUE_BASE_POINT0, static_cast<int32>(bp0));
        args.AddSpellMod(SPELLVALUE_BASE_POINT1, static_cast<int32>(bp1));
        caster->CastSpell(target, spellId, args);
    }

    // ---- Find player by name ----
    static Player* Global_FindPlayerByName(const std::string& name)
    {
        return ObjectAccessor::FindPlayerByName(name);
    }

    static std::string Global_GetConfigString(const std::string& key, const std::string& defaultVal)
    {
        return sConfigMgr->GetStringDefault(key, defaultVal);
    }

    static Player* Global_FindPlayerByGUID(uint64 guidRaw)
    {
        ObjectGuid guid;
        guid.SetRawValue(0, guidRaw);
        return ObjectAccessor::FindPlayer(guid);
    }

    // ---- Math helpers ----
    static float Math_Abs(float v) { return std::abs(v); }
    static float Math_Sqrt(float v) { return std::sqrt(v); }
    static float Math_Sin(float v) { return std::sin(v); }
    static float Math_Cos(float v) { return std::cos(v); }
    static float Math_Tan(float v) { return std::tan(v); }
    static float Math_ATan2(float y, float x) { return std::atan2(y, x); }
    static float Math_Pow(float base, float exp) { return std::pow(base, exp); }
    static float Math_Min(float a, float b) { return std::min(a, b); }
    static float Math_Max(float a, float b) { return std::max(a, b); }
    static int32 Math_MinInt(int32 a, int32 b) { return std::min(a, b); }
    static int32 Math_MaxInt(int32 a, int32 b) { return std::max(a, b); }
    static uint32 Math_MinUInt(uint32 a, uint32 b) { return std::min(a, b); }
    static uint32 Math_MaxUInt(uint32 a, uint32 b) { return std::max(a, b); }
    static float Math_Clamp(float v, float lo, float hi) { return std::clamp(v, lo, hi); }
    static float Math_Dist3D(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // ---- AngelScript Engine Memory Statistics ----
    static uint32_t GetASModuleCount()
    {
        if (!sAngelScriptMgr || !sAngelScriptMgr->GetEngine()) return 0;
        return sAngelScriptMgr->GetEngine()->GetModuleCount();
    }

    static std::string GetASModuleName(uint32_t index)
    {
        if (!sAngelScriptMgr || !sAngelScriptMgr->GetEngine()) return "";
        asIScriptModule* mod = sAngelScriptMgr->GetEngine()->GetModuleByIndex(index);
        return mod ? std::string(mod->GetName()) : "";
    }

    static uint32_t GetASModuleFunctionCount(uint32_t index)
    {
        if (!sAngelScriptMgr || !sAngelScriptMgr->GetEngine()) return 0;
        asIScriptModule* mod = sAngelScriptMgr->GetEngine()->GetModuleByIndex(index);
        return mod ? static_cast<uint32_t>(mod->GetFunctionCount()) : 0;
    }

    static uint32_t GetASGCObjectCount()
    {
        if (!sAngelScriptMgr || !sAngelScriptMgr->GetEngine()) return 0;
        asUINT currentSize = 0;
        sAngelScriptMgr->GetEngine()->GetGCStatistics(&currentSize);
        return currentSize;
    }

    static std::string GetASStatsSummary()
    {
        if (!sAngelScriptMgr || !sAngelScriptMgr->GetEngine()) return "AS: not initialized";
        
        asIScriptEngine* engine = sAngelScriptMgr->GetEngine();
        asUINT gcSize = 0;
        engine->GetGCStatistics(&gcSize);
        
        uint32_t moduleCount = engine->GetModuleCount();
        uint32_t totalFuncs = 0;
        for (uint32_t i = 0; i < moduleCount; i++)
        {
            asIScriptModule* mod = engine->GetModuleByIndex(i);
            if (mod) totalFuncs += mod->GetFunctionCount();
        }
        
        // Estimate memory: ~1KB per function + overhead
        uint32_t estimatedKB = (totalFuncs * 1) + (moduleCount * 10) + (gcSize / 100);
        
        return "[AS] " + std::to_string(moduleCount) + " modules, " + 
               std::to_string(totalFuncs) + " funcs, " + 
               std::to_string(gcSize) + " gc objs, ~" + 
               std::to_string(estimatedKB) + "KB";
    }

    void RegisterGlobalFunctions(asIScriptEngine* _scriptEngine)
    {
        int r;
        // Print / debug
        r = _scriptEngine->RegisterGlobalFunction("void Print(const string& in)", asFUNCTION(Global_Print), asCALL_CDECL);
        if (r < 0)
            TC_LOG_ERROR("server.angelscript", "RegisterGlobalFunction Print failed: {}", r);
        r = _scriptEngine->RegisterGlobalFunction("void SendSystemMessage(Player@, const string& in)", asFUNCTION(Global_SendSystemMessage), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void SendFloatingText(Player@, const string& in, uint32)", asFUNCTION(Global_SendFloatingText), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void PlaySoundToPlayer(Player@, uint32)", asFUNCTION(Global_PlaySoundToPlayer), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("void SendWorldText(const string& in)", asFUNCTION(Global_SendWorldText), asCALL_CDECL);

        // Time
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetWorldTime()", asFUNCTION(World_GetTime), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetGameTime()", asFUNCTION(World_GetGameTime), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetMSTime()", asFUNCTION(World_GetMSTime), asCALL_CDECL);

        // World config / player count
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetWorldConfigUInt32(uint32)", asFUNCTION(World_GetConfigUInt32), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("float GetWorldConfigFloat(uint32)", asFUNCTION(World_GetConfigFloat), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("bool GetWorldConfigBool(uint32)", asFUNCTION(World_GetConfigBool), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetPlayerCount()", asFUNCTION(World_GetPlayerCount), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetMaxPlayerCount()", asFUNCTION(World_GetMaxPlayerCount), asCALL_CDECL);

        // Player finders
        r = _scriptEngine->RegisterGlobalFunction("Player@ FindPlayerByName(const string& in)", asFUNCTION(Global_FindPlayerByName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetConfigString(const string& in, const string& in)", asFUNCTION(Global_GetConfigString), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("Player@ FindPlayerByGUID(uint64)", asFUNCTION(Global_FindPlayerByGUID), asCALL_CDECL);

        // Spell casting with custom base points
        r = _scriptEngine->RegisterGlobalFunction("void CastSpellWithBP(Unit@, Unit@, uint32, float, float)", asFUNCTION(Global_CastSpellWithBP), asCALL_CDECL);

        // AngelScript memory statistics
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetAngelScriptModuleCount()", asFUNCTION(GetASModuleCount), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetAngelScriptModuleName(uint32)", asFUNCTION(GetASModuleName), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetAngelScriptModuleFunctionCount(uint32)", asFUNCTION(GetASModuleFunctionCount), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 GetAngelScriptGCSize()", asFUNCTION(GetASGCObjectCount), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("string GetAngelScriptStatsSummary()", asFUNCTION(GetASStatsSummary), asCALL_CDECL);

        TC_LOG_INFO("server.angelscript", "Global functions registered");
    }

    void RegisterMathAPI(asIScriptEngine* _scriptEngine)
    {
        int r;
        // abs/sqrt/sin/cos/tan/atan2/pow are provided by the scriptmath add-on
        r = _scriptEngine->RegisterGlobalFunction("float Min(float, float)", asFUNCTION(Math_Min), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("float Max(float, float)", asFUNCTION(Math_Max), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("int32 MinI(int32, int32)", asFUNCTION(Math_MinInt), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("int32 MaxI(int32, int32)", asFUNCTION(Math_MaxInt), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 MinU(uint32, uint32)", asFUNCTION(Math_MinUInt), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("uint32 MaxU(uint32, uint32)", asFUNCTION(Math_MaxUInt), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("float Clamp(float, float, float)", asFUNCTION(Math_Clamp), asCALL_CDECL);
        r = _scriptEngine->RegisterGlobalFunction("float Dist3D(float, float, float, float, float, float)", asFUNCTION(Math_Dist3D), asCALL_CDECL);

        // Constants
        r = _scriptEngine->RegisterGlobalProperty("const float PI", const_cast<float*>(&Math_PI));
        r = _scriptEngine->RegisterGlobalProperty("const float HALF_PI", const_cast<float*>(&Math_Half_PI));

        TC_LOG_INFO("server.angelscript", "Math API registered");
    }

    // ---- String split utility for BattlePay ----
    static CScriptArray* String_Split(const std::string& str, const std::string& delimiter)
    {
        asIScriptContext* ctx = asGetActiveContext();
        asIScriptEngine* engine = ctx ? ctx->GetEngine() : nullptr;
        if (!engine) return nullptr;
        
        asITypeInfo* arrayType = engine->GetTypeInfoByDecl("array<string>");
        CScriptArray* arr = CScriptArray::Create(arrayType);
        
        if (str.empty()) return arr;
        
        size_t start = 0;
        size_t end = str.find(delimiter);
        while (end != std::string::npos)
        {
            arr->Resize(arr->GetSize() + 1);
            std::string token = str.substr(start, end - start);
            ((std::string*)arr->At(arr->GetSize() - 1))->assign(token);
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        arr->Resize(arr->GetSize() + 1);
        ((std::string*)arr->At(arr->GetSize() - 1))->assign(str.substr(start));
        
        return arr;
    }

    // ---- Time utilities ----
    static uint32 Global_GetUnixTime()
    {
        return uint32(GameTime::GetGameTime());
    }

    void RegisterStringAPI(asIScriptEngine* _scriptEngine)
    {
        // Register string split method
        _scriptEngine->RegisterObjectMethod("string", "array<string>@ split(const string &in) const", 
            asFUNCTION(String_Split), asCALL_CDECL_OBJFIRST);
        
        // Register global time function
        _scriptEngine->RegisterGlobalFunction("uint32 GetUnixTime()", asFUNCTION(Global_GetUnixTime), asCALL_CDECL);
        
        TC_LOG_INFO("server.angelscript", "String API registered (with split method)");
    }

} // namespace AngelScript
