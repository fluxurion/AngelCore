#ifndef ANGELSCRIPT_INTEGRATION
    #error "ANGELSCRIPT_INTEGRATION macro must be defined"
#endif
#ifdef ANGELSCRIPT_INTEGRATION
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
#include <scriptstdstring/scriptstdstring.h>
#include <scriptarray/scriptarray.h>
#include <scripthelper/scripthelper.h>
#include <scriptmath/scriptmath.h>
#include <scriptbuilder/scriptbuilder.h>
#pragma pop_macro("OPTIONAL")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")
#include "AngelScriptMgr.h"
#include "ASPacketData.h"
#include "Log.h"
#include "World.h"
#include "Player.h"
#include "Creature.h"
#include "Map.h"
#include "MapManager.h"
#include "InstanceScript.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ByteBuffer.h"
#include "Server/Packets/CharacterPackets.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "Language.h"
#include "API/ASPlayerAPI.h"
#include "API/ASCreatureAPI.h"
#include "API/ASCharacterPacketsAPI.h"
#include "API/ASGameObjectAPI.h"
#include "API/ASUnitAPI.h"
#include "API/ASAngelDB.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "API/ASSpellAPI.h"
#include "API/ASGlobalFunctions.h"
#include "API/ASDatabaseAPI.h"
#include "API/ASGossipAPI.h"
#include "API/ASPacketAPI.h"
#include "API/ASDB2API.h"
#include "API/ASWorldAPI.h"
#include "API/ASUpdateFieldAPI.h"
#include "API/ASSpawnAPI.h"
#include "Dispatch/ASDispatch.h"
#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include <algorithm>
#include <cstdio>
namespace fs = std::filesystem;
namespace AngelScript
{
AngelScriptMgr* AngelScriptMgr::instance() { static AngelScriptMgr inst; return &inst; }
AngelScriptMgr::AngelScriptMgr() : _scriptEngine(nullptr), _scriptBuilder(nullptr), _context(nullptr), _enabled(false), _scriptPath("angelscripts"), _scriptCachePath("angelscripts/cache") {}
AngelScriptMgr::~AngelScriptMgr() { Shutdown(); }
bool AngelScriptMgr::InitializeAndReturn() { Initialize(); return _enabled; }

void AngelScriptMgr::Initialize()
{
    if (_enabled) return;
    TC_LOG_INFO("server.loading", ">> AngelScript: initializing (script path: '{}')", _scriptPath);
    if (!SetupEngine()) { TC_LOG_ERROR("server.loading", ">> AngelScript: engine setup FAILED"); return; }
    InitializeDB2Loader();
    if (!LoadScripts())
        TC_LOG_WARN("server.loading", ">> AngelScript: some scripts failed to load (check log for details)");
    _enabled = true;
    RegisterDispatchScripts();
    TC_LOG_INFO("server.loading", ">> AngelScript: initialized with {} module(s) loaded", _modules.size());
    TriggerWorldHook(WorldHookType::ON_STARTUP);
}

void AngelScriptMgr::Shutdown()
{
    if (!_enabled) return;
    TriggerWorldHook(WorldHookType::ON_SHUTDOWN);
    UnloadScripts();
    _creatureAIFactories.clear(); _gameObjectAIFactories.clear(); _instanceScripts.clear();
    if (_context) { _context->Release(); _context = nullptr; }
    if (_scriptBuilder) { delete _scriptBuilder; _scriptBuilder = nullptr; }
    if (_scriptEngine) { _scriptEngine->ShutDownAndRelease(); _scriptEngine = nullptr; }
    _enabled = false;
}

void AngelScriptMgr::ReloadScripts()
{
    if (!_enabled) return;
    TC_LOG_INFO("server.angelscript", "Reloading AngelScript scripts...");
    ASWorldHooks::instance()->Clear(); ASPlayerHooks::instance()->Clear();
    ASCreatureHooks::instance()->Clear(); ASGameObjectHooks::instance()->Clear();
    ASSpellHooks::instance()->Clear(); ASQuestHooks::instance()->Clear();
    ASCraftingHooks::instance()->Clear(); ASPacketHooks::instance()->Clear();
    ASInstanceHooks::instance()->Clear(); ASBattlegroundHooks::instance()->Clear();
    for (auto& hooks : _customHooks) hooks.clear();
    _creatureAIFactories.clear(); _gameObjectAIFactories.clear();
    UnloadScripts(); LoadScripts();
    TC_LOG_INFO("server.angelscript", "Reload complete ({} modules)", _modules.size());
}

void AngelScriptMgr::ReloadScript(const std::string& name)
{
    if (!_enabled) return;
    std::string fn = name; if (fn.find(".as") == std::string::npos) fn += ".as";
    std::string path = _scriptPath + "/" + fn;
    if (!fs::exists(path)) { TC_LOG_ERROR("server.angelscript", "Script not found: {}", path); return; }
    auto it = _modules.find(fn);
    if (it != _modules.end()) { if (it->second) it->second->Discard(); _scriptEngine->DiscardModule(fn.c_str()); _modules.erase(it); }
    CompileScript(path, fn);
}

void AngelScriptMgr::GetStatus(std::string& output)
{
    if (!_enabled) { output = "AngelScript not initialized"; return; }
    std::stringstream ss;
    ss << "AngelScript: Enabled, Modules: " << _modules.size()
       << ", CreatureAI: " << _creatureAIFactories.size()
       << ", GOAI: " << _gameObjectAIFactories.size() << "\n";
    for (auto& p : _modules) ss << "  - " << p.first << "\n";
    output = ss.str();
}

bool AngelScriptMgr::SetupEngine()
{
    _scriptEngine = asCreateScriptEngine();
    if (!_scriptEngine) return false;
    _scriptEngine->SetMessageCallback(asFUNCTION(+[](const asSMessageInfo* msg, void*) {
        switch (msg->type)
        {
        case asMSGTYPE_ERROR:       TC_LOG_ERROR("server.angelscript", "[AS-ERR] {}({}): {}", msg->section, msg->row, msg->message); break;
        case asMSGTYPE_WARNING:     TC_LOG_WARN ("server.angelscript", "[AS-WRN] {}({}): {}", msg->section, msg->row, msg->message); break;
        case asMSGTYPE_INFORMATION: TC_LOG_INFO ("server.angelscript", "[AS-INF] {}({}): {}", msg->section, msg->row, msg->message); break;
        }
    }), nullptr, asCALL_CDECL);
    RegisterStandardAddons(); RegisterTrinityCoreAPI();
    _scriptBuilder = new CScriptBuilder(); _scriptBuilder->StartNewModule(_scriptEngine, "trinitycore");
    _context = _scriptEngine->CreateContext();
    return _context != nullptr;
}

void AngelScriptMgr::RegisterStandardAddons()
{
    RegisterStdString(_scriptEngine); RegisterScriptArray(_scriptEngine, true); RegisterScriptMath(_scriptEngine);
}

void AngelScriptMgr::RegisterTrinityCoreAPI()
{
    // ---- Pre-register all reference types ----
    // Types must exist before ANY funcdef or method referencing them is registered.
    // Player/Creature/GameObject reference Unit@ and vice-versa (circular dependency).
    // Each API module also calls RegisterObjectType; asALREADY_REGISTERED is tolerated.
    for (char const* t : { "Unit", "Player", "Creature", "GameObject", "Spell", "AuraEffect",
                           "PacketData", "QueryResult", "WorldObject", "WorldSession", "EnumCharactersResult", "CharEnumCharacterInfo",
                           "DB2Schema", "DB2Storage", "DB2Record" })
        _scriptEngine->RegisterObjectType(t, 0, asOBJ_REF | asOBJ_NOCOUNT);

    // ---- Funcdefs (must come AFTER type pre-registration since they reference types) ----
    _scriptEngine->RegisterFuncdef("void ScriptCallback()");
    _scriptEngine->RegisterFuncdef("void PlayerCallback(Player@)");
    _scriptEngine->RegisterFuncdef("void CreatureCallback(Creature@)");
    _scriptEngine->RegisterFuncdef("void SpellCallback(Spell@)");
    _scriptEngine->RegisterFuncdef("void SpellHitCallback(Spell@, Unit@)");  // per-spell ON_HIT
    _scriptEngine->RegisterFuncdef("void SpellCalcCallback(Spell@, uint8, Unit@, int32&out, int32&out, float&out)");
    _scriptEngine->RegisterFuncdef("void AuraCalcAmountCallback(AuraEffect@, double&out, bool&out)");
    _scriptEngine->RegisterFuncdef("void AuraApplyCallback(Unit@, AuraEffect@)");  // ON_AURA_APPLY
    _scriptEngine->RegisterFuncdef("void UnitCallback(Unit@)");
    _scriptEngine->RegisterFuncdef("void GameObjectCallback(GameObject@)");
    _scriptEngine->RegisterFuncdef("void PlayerPlayerCallback(Player@, Player@)");
    _scriptEngine->RegisterFuncdef("void StringCallback(string &in)");
    _scriptEngine->RegisterFuncdef("void CharEnumCallback(WorldSession@, EnumCharactersResult@)");
    _scriptEngine->RegisterFuncdef("void SessionInitializedCallback(WorldSession@)");

    RegisterPlayerAPI(); RegisterCreatureAPI(); RegisterGameObjectAPI(); RegisterUnitAPI();
    RegisterSpellAPI(); RegisterAuraAPI(); RegisterPacketAPI(); RegisterDatabaseAPI(); RegisterGossipAPI();
    RegisterCharacterPacketsAPI();
    RegisterGlobalFunctions(); RegisterWorldAPI(); RegisterUpdateFieldAPI(); RegisterMathAPI(); RegisterStringAPI();
    RegisterQuestAPI(); RegisterCraftingAPI(); RegisterItemAPI(); RegisterDB2API(); RegisterDynamicDB2API();
    RegisterSpawnAPI(); RegisterAngelDBAPI();

    // ---- Hook registration functions exposed to scripts ----
    // No-arg callbacks (ScriptCallback)
    _scriptEngine->RegisterGlobalFunction("void RegisterWorldScript(int, ScriptCallback@)",        asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterWorldScript(static_cast<WorldHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterBattlegroundScript(int, ScriptCallback@)", asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterBattlegroundScript(static_cast<BattlegroundHookType>(t), f); }), asCALL_CDECL);
    // Console command hook (StringCallback - receives the command string)
    _scriptEngine->RegisterGlobalFunction("void RegisterConsoleCommandHook(StringCallback@)", asFUNCTION(+[](asIScriptFunction* f){ sAngelScriptMgr->RegisterWorldScript(WorldHookType::ON_CONSOLE_COMMAND, f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterPacketScript(int, ScriptCallback@)",       asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterPacketScript(static_cast<PacketHookType>(t), f); }), asCALL_CDECL);

    // Player callbacks
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellCalcHealingHook(uint32, SpellCalcCallback@)", asFUNCTION(+[](uint32 id, asIScriptFunction* f){ ASSpellHooks::instance()->RegisterSpellHook(id, SpellHookType::ON_HEAL_CALC, f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterAuraCalcAmountHook(uint32, AuraCalcAmountCallback@)", asFUNCTION(+[](uint32 id, asIScriptFunction* f){ ASSpellHooks::instance()->RegisterSpellHook(id, SpellHookType::ON_AURA_CALC_AMOUNT, f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterPlayerHook(int, PlayerCallback@)",       asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterPlayerScript(static_cast<PlayerHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterCharEnumHook(CharEnumCallback@)", asFUNCTION(+[](asIScriptFunction* f){ sAngelScriptMgr->RegisterCustomHook(CustomHookType::ON_CHAR_ENUM, f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterSessionInitializedHook(SessionInitializedCallback@)", asFUNCTION(+[](asIScriptFunction* f){ sAngelScriptMgr->RegisterCustomHook(CustomHookType::ON_SESSION_INITIALIZED, f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterInstanceScript(int, PlayerCallback@)",     asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterInstanceScript(static_cast<InstanceHookType>(t), f); }), asCALL_CDECL);

    // Creature callbacks
    _scriptEngine->RegisterGlobalFunction("void RegisterCreatureScript(int, CreatureCallback@)",   asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterCreatureScript(static_cast<CreatureHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterInstanceScript(int, CreatureCallback@)",   asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterInstanceScript(static_cast<InstanceHookType>(t), f); }), asCALL_CDECL);

    // GameObject callbacks
    _scriptEngine->RegisterGlobalFunction("void RegisterGameObjectScript(int, GameObjectCallback@)", asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterGameObjectScript(static_cast<GameObjectHookType>(t), f); }), asCALL_CDECL);

    // Spell callbacks
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellHook(int, SpellCallback@)",           asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterSpellHook(static_cast<SpellHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellEffectHandler(uint32, uint8, SpellCallback@)", asFUNCTION(+[](uint32 id, uint8 idx, asIScriptFunction* f){ sAngelScriptMgr->RegisterSpellEffectHandler(id, idx, f); }), asCALL_CDECL);
    // Per-spell-ID hook with target: void handler(Spell@, Unit@)
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellScript(uint32, int, SpellHitCallback@)",
        asFUNCTION(+[](uint32 id, int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterSpellHook(id, static_cast<SpellHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellScript(uint32, int, SpellCalcCallback@)",
        asFUNCTION(+[](uint32 id, int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterSpellHook(id, static_cast<SpellHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellScript(uint32, int, AuraCalcAmountCallback@)",
        asFUNCTION(+[](uint32 id, int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterSpellHook(id, static_cast<SpellHookType>(t), f); }), asCALL_CDECL);
    // Aura apply callback (Spell@, Unit@, AuraEffect@)
    _scriptEngine->RegisterGlobalFunction("void RegisterSpellScript(uint32, int, AuraApplyCallback@)",
        asFUNCTION(+[](uint32 id, int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterSpellHook(id, static_cast<SpellHookType>(t), f); }), asCALL_CDECL);

    // Unit callbacks
    _scriptEngine->RegisterGlobalFunction("void RegisterPlayerScript(int, PlayerCallback@)",       asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterPlayerScript(static_cast<PlayerHookType>(t), f); }), asCALL_CDECL);
    _scriptEngine->RegisterGlobalFunction("void RegisterPlayerScript(int, UnitCallback@)",         asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterPlayerScript(static_cast<PlayerHookType>(t), f); }), asCALL_CDECL);

    // Player+Player callbacks
    _scriptEngine->RegisterGlobalFunction("void RegisterPlayerScript(int, PlayerPlayerCallback@)", asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterPlayerScript(static_cast<PlayerHookType>(t), f); }), asCALL_CDECL);

    RegisterSharedDataAPI(); RegisterScriptClassesAPI(); RegisterEnhancedPacketAPI();
    RegisterScriptAttributesAPI(); RegisterInstanceAPI(); RegisterBattlegroundAPI();
    RegisterArenaAPI(); RegisterMapAPI(); RegisterGroupGuildAPI(); RegisterItemAuctionAPI();
    TC_LOG_INFO("server.angelscript", "TrinityCore API registered");
}

// Delegates to API/ files
void AngelScriptMgr::RegisterPlayerAPI()      { AngelScript::RegisterPlayerAPI(_scriptEngine); }
void AngelScriptMgr::RegisterCreatureAPI()    { AngelScript::RegisterCreatureAPI(_scriptEngine); }
void AngelScriptMgr::RegisterGameObjectAPI()  { AngelScript::RegisterGameObjectAPI(_scriptEngine); }
void AngelScriptMgr::RegisterUnitAPI()        { AngelScript::RegisterUnitAPI(_scriptEngine); }
void AngelScriptMgr::RegisterAuraAPI()        { AngelScript::RegisterAuraAPI(_scriptEngine); }
void AngelScriptMgr::RegisterSpellAPI()       { AngelScript::RegisterSpellAPI(_scriptEngine); }
void AngelScriptMgr::RegisterPacketAPI()      { AngelScript::RegisterPacketAPI(_scriptEngine); }
void AngelScriptMgr::RegisterDatabaseAPI()    { AngelScript::RegisterDatabaseAPI(_scriptEngine); }
void AngelScriptMgr::RegisterGossipAPI()      { AngelScript::RegisterGossipAPI(_scriptEngine); }
void AngelScriptMgr::RegisterGlobalFunctions(){ AngelScript::RegisterGlobalFunctions(_scriptEngine); }
void AngelScriptMgr::RegisterWorldAPI()       { AngelScript::RegisterWorldAPI(_scriptEngine); }
void AngelScriptMgr::RegisterUpdateFieldAPI()  { AngelScript::RegisterUpdateFieldAPI(_scriptEngine); }
void AngelScriptMgr::RegisterMathAPI()        { AngelScript::RegisterMathAPI(_scriptEngine); }
void AngelScriptMgr::RegisterStringAPI()      { AngelScript::RegisterStringAPI(_scriptEngine); }
void AngelScriptMgr::RegisterQuestAPI()       { _scriptEngine->RegisterGlobalFunction("void RegisterQuestScript(int, ScriptCallback@)", asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterQuestScript(static_cast<QuestHookType>(t), f); }), asCALL_CDECL); }
void AngelScriptMgr::RegisterCraftingAPI()    { _scriptEngine->RegisterGlobalFunction("void RegisterCraftingScript(int, ScriptCallback@)", asFUNCTION(+[](int t, asIScriptFunction* f){ sAngelScriptMgr->RegisterCraftingScript(static_cast<CraftingHookType>(t), f); }), asCALL_CDECL); }
void AngelScriptMgr::RegisterItemAPI() {}
void AngelScriptMgr::RegisterDB2API() { AngelScript::RegisterDB2API(_scriptEngine); }
void AngelScriptMgr::RegisterSpawnAPI() { AngelScript::RegisterSpawnAPI(_scriptEngine); }
void AngelScriptMgr::RegisterAngelDBAPI() { AngelScript::RegisterAngelDBAPI(_scriptEngine); }
void AngelScriptMgr::RegisterDynamicDB2API() { AngelScript::RegisterDynamicDB2API(_scriptEngine); }
void AngelScriptMgr::InitializeDB2Loader() { /* DB2 data is already loaded by TC at startup — no custom loading needed */ }
void AngelScriptMgr::RegisterSharedDataAPI() {}
void AngelScriptMgr::RegisterScriptClassesAPI() {}
void AngelScriptMgr::RegisterEnhancedPacketAPI() {}
void AngelScriptMgr::RegisterScriptAttributesAPI() {}
void AngelScriptMgr::RegisterInstanceAPI() {}
void AngelScriptMgr::RegisterBattlegroundAPI() {}
void AngelScriptMgr::RegisterArenaAPI() {}
void AngelScriptMgr::RegisterMapAPI() {}
void AngelScriptMgr::RegisterGroupGuildAPI() {}
void AngelScriptMgr::RegisterItemAuctionAPI() {}

// Per-entry AI - undef TC macros to prevent conflicts
#undef RegisterCreatureAI
#undef RegisterGameObjectAI
void AngelScriptMgr::RegisterCreatureAI(uint32 entry, asIScriptFunction* f) { if(f) _creatureAIFactories[entry]=f; }
void AngelScriptMgr::RegisterGameObjectAI(uint32 entry, asIScriptFunction* f) { if(f) _gameObjectAIFactories[entry]=f; }
CreatureAI* AngelScriptMgr::GetCreatureAI(Creature* c)
{
    if (!c) return nullptr;
    auto it = _creatureAIFactories.find(c->GetEntry());
    if (it == _creatureAIFactories.end()) return nullptr;
    // TODO: Create a CreatureAI proxy that dispatches to the script function
    // For now, return nullptr to let TC use default AI
    // The hook system (TriggerCreatureHook) handles broadcast hooks instead
    return nullptr;
}

GameObjectAI* AngelScriptMgr::GetGameObjectAI(GameObject* go)
{
    if (!go) return nullptr;
    auto it = _gameObjectAIFactories.find(go->GetEntry());
    if (it == _gameObjectAIFactories.end()) return nullptr;
    // TODO: Create a GameObjectAI proxy that dispatches to the script function
    return nullptr;
}

InstanceScript* AngelScriptMgr::GetInstanceScript(InstanceMap* m)
{
    if (!m) return nullptr;
    auto result = _instanceScripts.find(m->GetInstanceId());
    if (result == _instanceScripts.end())
        return nullptr;
    // ASInstanceScript is not derived from InstanceScript, so we cannot return it directly.
    // The hook system (TriggerInstanceHook) handles broadcast hooks instead.
    return nullptr;
}

// Script loading
bool AngelScriptMgr::LoadScripts()
{
    if (!_scriptEngine) return false;

    // Resolve script path relative to the binary directory if not absolute
    fs::path scriptDir = fs::path(_scriptPath);
    if (!scriptDir.is_absolute())
    {
        // Get the directory containing the running executable
        fs::path exeDir;
#ifdef _WIN32
        char exeBuf[MAX_PATH];
        GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
        exeDir = fs::path(exeBuf).parent_path();
#else
        exeDir = fs::read_symlink("/proc/self/exe").parent_path();
#endif
        scriptDir = exeDir / scriptDir;
        _scriptPath = scriptDir.string();
    }

    TC_LOG_INFO("server.loading", ">> AngelScript: looking for scripts in '{}'", fs::absolute(_scriptPath).string());

    if (!fs::exists(_scriptPath))
    {
        TC_LOG_WARN("server.loading", ">> AngelScript: script directory '{}' does not exist, creating it", _scriptPath);
        fs::create_directories(_scriptPath);
        return false;
    }

    uint32 ok=0,fail=0;
    for (auto& e : fs::recursive_directory_iterator(_scriptPath))
    {
        if (!e.is_regular_file() || e.path().extension() != ".as")
            continue;
        // Skip includes and examples directories
        std::string parentDir = e.path().parent_path().filename().string();
        if (parentDir == "includes" || parentDir == "examples")
            continue;
        // Also skip anything under examples/ at any depth
        std::string relPath = fs::relative(e.path(), _scriptPath).string();
        if (relPath.find("examples/") == 0 || relPath.find("examples\\") == 0)
            continue;
        // Skip files with "example" in the name (e.g., quest_encounter_example.as)
        std::string filename = e.path().filename().string();
        if (filename.find("example") != std::string::npos)
            continue;
        CompileScript(e.path().string(), fs::relative(e.path(),_scriptPath).string()) ? ok++ : fail++;
    }
    TC_LOG_INFO("server.loading", ">> AngelScript: compiled {} script(s), {} failed", ok, fail);
    return fail==0;
}
bool AngelScriptMgr::LoadAllScripts() { return LoadScripts(); }
void AngelScriptMgr::UnloadScripts() { for(auto& p:_modules) if(p.second) p.second->Discard(); _modules.clear(); if(_scriptEngine) _scriptEngine->GarbageCollect(asGC_FULL_CYCLE); }
void AngelScriptMgr::UnloadAllScripts() { UnloadScripts(); }

bool AngelScriptMgr::CompileScript(const std::string& filename, const std::string& moduleName)
{
    CScriptBuilder builder;

    // --- Defense mechanism: detailed error reporting ---
    // Set message callback to capture compiler messages
    class ErrorCollector
    {
    public:
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        void Callback(asSMessageInfo* msg)
        {
            std::string sev;
            switch (msg->type)
            {
            case asMSGTYPE_ERROR:   sev = "ERR"; errors.push_back(fmt::format("{}({}): {}", msg->section, msg->row, msg->message)); break;
            case asMSGTYPE_WARNING: sev = "WRN"; warnings.push_back(fmt::format("{}({}): {}", msg->section, msg->row, msg->message)); break;
            default:                sev = "INF"; break;
            }
            TC_LOG_INFO("server.angelscript", "[AS-{}] {}({}): {}", sev, msg->section, msg->row, msg->message);
        }
    } collector;

    _scriptEngine->SetMessageCallback(asMETHOD(ErrorCollector, Callback), &collector, asCALL_THISCALL);

    int r = builder.StartNewModule(_scriptEngine, moduleName.c_str());
    if (r < 0)
    {
        TC_LOG_ERROR("server.angelscript", "[DEFENSE] Failed to start module '{}' - engine error {}", moduleName, r);
        _scriptEngine->ClearMessageCallback();
        return false;
    }

    r = builder.AddSectionFromFile(filename.c_str());
    if (r < 0)
    {
        TC_LOG_ERROR("server.angelscript", "[DEFENSE] Failed to read script file '{}' - file not found or unreadable", filename);
        _scriptEngine->ClearMessageCallback();
        return false;
    }

    r = builder.BuildModule();
    if (r < 0)
    {
        TC_LOG_ERROR("server.loading", ">> AngelScript: '{}' FAILED to compile ({} errors, {} warnings)", filename, collector.errors.size(), collector.warnings.size());
        for (auto const& e : collector.errors)
            TC_LOG_ERROR("server.loading", ">>     {}", e);
        for (auto const& w : collector.warnings)
            TC_LOG_WARN ("server.loading", ">>     {}", w);
        // Discard the failed module to free resources
        asIScriptModule* badMod = _scriptEngine->GetModule(moduleName.c_str());
        if (badMod) badMod->Discard();
        _scriptEngine->ClearMessageCallback();
        return false;
    }

    // Log warnings even on success
    if (!collector.warnings.empty())
    {
        TC_LOG_WARN("server.angelscript", "Script '{}' compiled with {} warnings:", filename, collector.warnings.size());
        for (auto const& w : collector.warnings)
            TC_LOG_WARN("server.angelscript", "  WARNING: {}", w);
    }

    _scriptEngine->ClearMessageCallback();

    asIScriptModule* mod = _scriptEngine->GetModule(moduleName.c_str());
    if (!mod) return false;
    _modules[moduleName] = mod;

    // Execute entry points if they exist
    if (asIScriptFunction* f = mod->GetFunctionByDecl("void main()"))
    {
        if (!ExecuteScriptFunction(f))
            TC_LOG_ERROR("server.angelscript", "[DEFENSE] main() in '{}' failed to execute", filename);
    }
    if (asIScriptFunction* f = mod->GetFunctionByDecl("void RegisterHooks()"))
    {
        if (!ExecuteScriptFunction(f))
            TC_LOG_ERROR("server.angelscript", "[DEFENSE] RegisterHooks() in '{}' failed to execute", filename);
    }

    return true;
}

// Hook registration
void AngelScriptMgr::RegisterWorldScript(WorldHookType t, asIScriptFunction* f) { ASWorldHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterPlayerScript(PlayerHookType t, asIScriptFunction* f) { ASPlayerHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterCreatureScript(CreatureHookType t, asIScriptFunction* f) { ASCreatureHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterGameObjectScript(GameObjectHookType t, asIScriptFunction* f) { ASGameObjectHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterSpellHook(SpellHookType t, asIScriptFunction* f) { ASSpellHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterSpellHook(uint32 id, SpellHookType t, asIScriptFunction* f) { ASSpellHooks::instance()->RegisterSpellHook(id,t,f); }
void AngelScriptMgr::RegisterSpellEffectHandler(uint32 id, uint8 idx, asIScriptFunction* f) { ASSpellHooks::instance()->RegisterEffectHandler(id,idx,f); }
void AngelScriptMgr::RegisterQuestScript(QuestHookType t, asIScriptFunction* f) { ASQuestHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterCraftingScript(CraftingHookType t, asIScriptFunction* f) { ASCraftingHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterPacketScript(PacketHookType t, asIScriptFunction* f) { ASPacketHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterInstanceScript(InstanceHookType t, asIScriptFunction* f) { ASInstanceHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterInstanceScript(uint32 id, ASInstanceScript* s) { ASInstanceHooks::instance()->RegisterInstanceScript(id,s); _instanceScripts[id]=s; }
void AngelScriptMgr::RegisterBattlegroundScript(BattlegroundHookType t, asIScriptFunction* f) { ASBattlegroundHooks::instance()->RegisterScript(t,f); }
void AngelScriptMgr::RegisterBattlegroundScript(uint32, ASBattlegroundScript*) {}
void AngelScriptMgr::RegisterArenaScript(ArenaHookType, asIScriptFunction*) {}
void AngelScriptMgr::RegisterArenaScript(uint32, ASArenaScript*) {}
void AngelScriptMgr::RegisterMapScript(MapHookType, asIScriptFunction*) {}
void AngelScriptMgr::RegisterGroupScript(GroupHookType, asIScriptFunction*) {}
void AngelScriptMgr::RegisterGuildScript(GuildHookType, asIScriptFunction*) {}
void AngelScriptMgr::RegisterItemScript(ItemHookType, asIScriptFunction*) {}
void AngelScriptMgr::RegisterAuctionScript(AuctionHookType, asIScriptFunction*) {}

// Custom hook points
void AngelScriptMgr::RegisterCustomHook(CustomHookType type, asIScriptFunction* func)
{
    if (!func) return;
    _customHooks[static_cast<size_t>(type)].push_back(func);
}

bool AngelScriptMgr::TriggerCustomHook_SendPlayerChoice(Player* player, int32 choiceId)
{
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_SEND_PLAYER_CHOICE)];
    for (auto& func : hooks)
    {
        if (!_context) break;
        int r = _context->Prepare(func);
        if (r < 0) continue;
        _context->SetArgObject(0, player);
        _context->SetArgDWord(1, static_cast<uint32>(choiceId));
        r = _context->Execute();
        if (r == asEXECUTION_FINISHED)
        {
            // If script returns true, the event is handled (override)
            if (_context->GetReturnByte())
                return true;
        }
    }
    return false;
}

bool AngelScriptMgr::TriggerCustomHook_CharEnum(WorldSession* session, WorldPackets::Character::EnumCharactersResult& enumResult)
{
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_CHAR_ENUM)];
    for (auto& func : hooks)
    {
        if (!_context) break;
        int r = _context->Prepare(func);
        if (r < 0) continue;
        _context->SetArgObject(0, session);
        _context->SetArgObject(1, &enumResult);
        r = _context->Execute();
        if (r == asEXECUTION_EXCEPTION)
            TC_LOG_ERROR("server.angelscript", "[AS] CharEnum EXCEPTION: {} at {}:{}",
                _context->GetExceptionString(),
                _context->GetExceptionFunction() ? _context->GetExceptionFunction()->GetName() : "?",
                _context->GetExceptionLineNumber());
    }
    return false;
}

bool AngelScriptMgr::TriggerCustomHook_AuthResponse(WorldSession* session, uint32& currencyID)
{
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_AUTH_RESPONSE)];
    for (auto& func : hooks)
    {
        if (!_context) break;
        int r = _context->Prepare(func);
        if (r < 0) continue;
        _context->SetArgDWord(0, session ? session->GetAccountId() : 0);
        _context->SetArgDWord(1, currencyID);
        r = _context->Execute();
        if (r == asEXECUTION_FINISHED)
        {
            // Script can modify currencyID via reference
            currencyID = _context->GetReturnDWord();
        }
        if (r == asEXECUTION_EXCEPTION)
            TC_LOG_ERROR("server.angelscript", "[AS] AuthResponse EXCEPTION: {} at {}:{}",
                _context->GetExceptionString(),
                _context->GetExceptionFunction() ? _context->GetExceptionFunction()->GetName() : "?",
                _context->GetExceptionLineNumber());
    }
    return hooks.size() > 0;
}

bool AngelScriptMgr::TriggerCustomHook_FeatureSystemStatusGlueScreen(WorldSession* session, bool& bpayStoreAvailable, int32& activeBoostType, bool& commerceServerEnabled, uint32& commercePricePollTimeSeconds, int32& contentSetID)
{
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_FEATURE_SYSTEM_STATUS_GLUE_SCREEN)];
    TC_LOG_DEBUG("server.angelscript", "[AS] FSS-Glue trigger called — {} hook(s) registered, context={} state={}",
        hooks.size(), (void*)_context, _context ? _context->GetState() : -1);
    for (auto& func : hooks)
    {
        if (!_context)
        {
            TC_LOG_ERROR("server.angelscript", "[AS] FSS-Glue — _context is null, breaking");
            break;
        }
        int stateBeforePrepare = _context->GetState();
        int r = _context->Prepare(func);
        int stateAfterPrepare = _context->GetState();
        TC_LOG_DEBUG("server.angelscript", "[AS] FSS-Glue Prepare(func={}) returned r={}, stateBefore={}, stateAfter={}",
            (void*)func, r, stateBeforePrepare, stateAfterPrepare);
        if (r < 0)
        {
            TC_LOG_ERROR("server.angelscript", "[AS] FSS-Glue — Prepare() returned {} (func={})", r, (void*)func);
            continue;
        }
        // Set input-only parameters (not &out)
        _context->SetArgDWord(0, session ? session->GetAccountId() : 0);
        _context->SetArgByte(1, bpayStoreAvailable ? 1 : 0);
        _context->SetArgDWord(2, static_cast<uint32>(activeBoostType));
        // &out params (3=bool, 4=uint32, 5=int32) do NOT need initial values —
        // setting them caused asEXECUTION_ERROR in AS 2.38.0
        r = _context->Execute();
        int stateAfterExecute = _context->GetState();
        TC_LOG_DEBUG("server.angelscript", "[AS] FSS-Glue Execute returned r={}, stateAfter={}", r, stateAfterExecute);
        if (r == asEXECUTION_FINISHED)
        {
            // Script can modify bpayStoreAvailable via return value
            uint8 retByte = _context->GetReturnByte();
            bpayStoreAvailable = retByte != 0;
            // Script can modify other values via reference parameters
            commerceServerEnabled = *reinterpret_cast<bool*>(_context->GetAddressOfArg(3));
            commercePricePollTimeSeconds = *reinterpret_cast<uint32*>(_context->GetAddressOfArg(4));
            contentSetID = *reinterpret_cast<int32*>(_context->GetAddressOfArg(5));
            TC_LOG_DEBUG("server.angelscript", "[AS] FSS-Glue EXECUTED OK — retByte={}, bpayStoreAvailable={}, commerceServer={}, pricePoll={}, contentSet={}",
                (int)retByte, bpayStoreAvailable, commerceServerEnabled, commercePricePollTimeSeconds, contentSetID);
        }
        else if (r == asEXECUTION_SUSPENDED)
        {
            TC_LOG_ERROR("server.angelscript", "[AS] FSS-Glue — Execute() returned asEXECUTION_SUSPENDED ({}); attempting resume", r);
            // Try resuming — simple hook shouldn't suspend, but try anyway
            int r2 = _context->Execute();
            TC_LOG_DEBUG("server.angelscript", "[AS] FSS-Glue resume attempt returned r2={}, stateAfter={}", r2, _context->GetState());
            if (r2 == asEXECUTION_FINISHED)
            {
                uint8 retByte = _context->GetReturnByte();
                bpayStoreAvailable = retByte != 0;
                commerceServerEnabled = *reinterpret_cast<bool*>(_context->GetAddressOfArg(3));
                commercePricePollTimeSeconds = *reinterpret_cast<uint32*>(_context->GetAddressOfArg(4));
                contentSetID = *reinterpret_cast<int32*>(_context->GetAddressOfArg(5));
                TC_LOG_DEBUG("server.angelscript", "[AS] FSS-Glue RESUMED OK — retByte={}, bpayStoreAvailable={}", (int)retByte, bpayStoreAvailable);
            }
        }
        else if (r == asEXECUTION_EXCEPTION)
            TC_LOG_ERROR("server.angelscript", "[AS] FSS-Glue EXCEPTION: {} at {}:{}",
                _context->GetExceptionString(),
                _context->GetExceptionFunction() ? _context->GetExceptionFunction()->GetName() : "?",
                _context->GetExceptionLineNumber());
        else
            TC_LOG_ERROR("server.angelscript", "[AS] FSS-Glue — Execute() returned unexpected code: {} (state after: {})", r, stateAfterExecute);
    }
    return hooks.size() > 0;
}

bool AngelScriptMgr::TriggerCustomHook_FeatureSystemStatus(WorldSession* session, bool& bpayStoreAvailable, bool& commerceServerEnabled, uint32& commercePricePollTimeSeconds)
{
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_FEATURE_SYSTEM_STATUS)];
    for (auto& func : hooks)
    {
        if (!_context) break;
        int r = _context->Prepare(func);
        if (r < 0) continue;
        _context->SetArgDWord(0, session ? session->GetAccountId() : 0);
        _context->SetArgByte(1, bpayStoreAvailable ? 1 : 0);
        r = _context->Execute();
        if (r == asEXECUTION_FINISHED)
        {
            // Script can modify bpayStoreAvailable via return value
            bpayStoreAvailable = _context->GetReturnByte() != 0;
            // Script can modify other values via reference parameters
            commerceServerEnabled = *reinterpret_cast<bool*>(_context->GetAddressOfArg(2));
            commercePricePollTimeSeconds = *reinterpret_cast<uint32*>(_context->GetAddressOfArg(3));
        }
        if (r == asEXECUTION_EXCEPTION)
            TC_LOG_ERROR("server.angelscript", "[AS] FeatureSystemStatus EXCEPTION: {} at {}:{}",
                _context->GetExceptionString(),
                _context->GetExceptionFunction() ? _context->GetExceptionFunction()->GetName() : "?",
                _context->GetExceptionLineNumber());
    }
    return hooks.size() > 0;
}

bool AngelScriptMgr::TriggerCustomHook_GetLockedDungeons(Player* player, std::vector<uint32>& /*lockedDungeons*/)
{
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_GET_LOCKED_DUNGEONS)];
    for (auto& func : hooks)
    {
        if (!_context) break;
        int r = _context->Prepare(func);
        if (r < 0) continue;
        _context->SetArgObject(0, player);
        r = _context->Execute();
        if (r == asEXECUTION_FINISHED)
        {
            if (_context->GetReturnByte())
                return true; // Script modified the lock list
        }
    }
    return false;
}

// Helper
bool AngelScriptMgr::ExecuteScriptFunction(asIScriptFunction* func)
{
    if (!func || !_context) return false;
    int r = _context->Prepare(func);
    if (r < 0) return false;
    r = _context->Execute();
    return r == asEXECUTION_FINISHED;
}

// ========================================================================
// Hook Triggering
// ========================================================================

#define EXEC_HOOKS(hooks, ...) do { for(auto& f : hooks) { if(!_context) break; if(_context->Prepare(f)<0) continue; __VA_ARGS__; _context->Execute(); } } while(0)

void AngelScriptMgr::TriggerWorldHook(WorldHookType t) { EXEC_HOOKS(ASWorldHooks::instance()->GetHooks(t)); }
void AngelScriptMgr::TriggerWorldUpdate(uint32 d) { for(auto& f:ASWorldHooks::instance()->GetHooks(WorldHookType::ON_UPDATE)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgDWord(0,d);_context->Execute();} }
void AngelScriptMgr::TriggerConsoleCommand(std::string& command) 
{
    // AngelScript reload commands — # prefix works from both console and in-game
    if (command == "reload angelscript" || command == "rel as" ||
        command == "#reload angelscript" || command == "#rel as")
    {
        ReloadScripts();
        printf("AngelScript scripts reloaded.\n");
        command.clear();
        return;
    }
    
    for(auto& f:ASWorldHooks::instance()->GetHooks(WorldHookType::ON_CONSOLE_COMMAND)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,&command);_context->Execute();} 
}

void AngelScriptMgr::TriggerPlayerHook(PlayerHookType t, Player* p) { if(!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(t), _context->SetArgObject(0,p)); }
void AngelScriptMgr::TriggerPlayerHook(PlayerHookType t, Player* p, Player* o) { if(!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(t), _context->SetArgObject(0,p); if(o)_context->SetArgObject(1,o)); }
void AngelScriptMgr::TriggerPlayerChat(Player* p, uint32 t, uint32 l, std::string& m) 
{ 
    if(!p)return;
    
    // AngelScript reload commands — # prefix works from both console and in-game
    if (m == "reload angelscript" || m == "rel as" ||
        m == "#reload angelscript" || m == "#rel as")
    {
        WorldSession* session = p->GetSession();
        if (session && !session->HasPermission(rbac::RBAC_PERM_COMMAND_RELOAD_ANGELSCRIPT))
        {
            ChatHandler handler(session);
            handler.SendSysMessage("Command not allowed for you.");
            m.clear();
            return;
        }

        ReloadScripts();
        
        ChatHandler handler(session);
        handler.SendSysMessage("AngelScript scripts reloaded.");
        m.clear();
        return;
    }

    // All # prefixed messages are AngelScript commands — strip # and route to ON_CHAT hooks,
    // then suppress the chat message so it doesn't broadcast as regular chat.
    bool isHashCommand = !m.empty() && m[0] == '#';
    if (isHashCommand)
    {
        m.erase(0, 1); // strip '#' so hooks see "bpay credits" instead of "#bpay credits"

        for (auto& f : ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_CHAT))
        {
            if (!_context) break;
            if (_context->Prepare(f) < 0) continue;
            _context->SetArgObject(0, p);
            _context->SetArgDWord(1, t);
            _context->SetArgDWord(2, l);
            _context->SetArgObject(3, &m);
            _context->Execute();
        }

        m.clear(); // suppress chat broadcast
        return;
    }
    
    for(auto& f:ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_CHAT)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,p);_context->SetArgDWord(1,t);_context->SetArgDWord(2,l);_context->SetArgObject(3,&m);_context->Execute();} 
}
void AngelScriptMgr::TriggerPlayerKill(Player* p, Unit* v) { if(!p||!v)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_KILL), _context->SetArgObject(0,p); _context->SetArgObject(1,v)); }
void AngelScriptMgr::TriggerPlayerDeath(Player* p, Unit* k) { if(!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_DEATH), _context->SetArgObject(0,p); if(k)_context->SetArgObject(1,k)); }
void AngelScriptMgr::TriggerPlayerLevelUp(Player* p, uint8 l) { if(!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_LEVEL_UP), _context->SetArgObject(0,p); _context->SetArgByte(1,l)); }
void AngelScriptMgr::TriggerPlayerDuelStart(Player* a, Player* b) { if(!a||!b)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_DUEL_START), _context->SetArgObject(0,a); _context->SetArgObject(1,b)); }
void AngelScriptMgr::TriggerPlayerDuelEnd(Player* w, Player* l, DuelCompleteType type) { if(!w||!l)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_DUEL_END), _context->SetArgObject(0,w); _context->SetArgObject(1,l); _context->SetArgDWord(2, static_cast<uint32>(type))); }
void AngelScriptMgr::TriggerPlayerMoneyChanged(Player* p, int64& a) { if(!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_MONEY_CHANGE), _context->SetArgObject(0,p); _context->SetArgQWord(1,static_cast<asQWORD>(a))); }
void AngelScriptMgr::TriggerPlayerGiveXP(Player* p, uint32& a, Unit* v) { if(!p)return; for(auto& f:ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_GIVE_XP)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,p);_context->SetArgDWord(1,a);if(v)_context->SetArgObject(2,v);_context->Execute();} }
void AngelScriptMgr::TriggerPlayerReputationChange(Player* p, uint32 fid, int32& s, bool inc) { if(!p)return; for(auto& f:ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_REPUTATION_CHANGE)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,p);_context->SetArgDWord(1,fid);_context->SetArgDWord(2,s);_context->SetArgByte(3,inc?1:0);_context->Execute();} }
void AngelScriptMgr::TriggerPlayerUpdateZone(Player* p, uint32 z, uint32 a) { if(!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_UPDATE_ZONE), _context->SetArgObject(0,p); _context->SetArgDWord(1,z); _context->SetArgDWord(2,a)); }

void AngelScriptMgr::TriggerCreatureHook(CreatureHookType t, Creature* c) { if(!c)return; EXEC_HOOKS(ASCreatureHooks::instance()->GetHooks(t), _context->SetArgObject(0,c)); }
void AngelScriptMgr::TriggerCreatureKillPlayer(Creature* c, Player* p) { if(!c||!p)return; EXEC_HOOKS(ASPlayerHooks::instance()->GetHooks(PlayerHookType::ON_KILLED_BY_CREATURE), _context->SetArgObject(0,c); _context->SetArgObject(1,p)); }

void AngelScriptMgr::TriggerGameObjectHook(GameObjectHookType t, GameObject* g) { if(!g)return; EXEC_HOOKS(ASGameObjectHooks::instance()->GetHooks(t), _context->SetArgObject(0,g)); }
void AngelScriptMgr::TriggerGameObjectUse(GameObject* g, Player* p) { if(!g||!p)return; EXEC_HOOKS(ASGameObjectHooks::instance()->GetHooks(GameObjectHookType::ON_USE), _context->SetArgObject(0,g); _context->SetArgObject(1,p)); }

void AngelScriptMgr::TriggerSpellHook(SpellHookType t, Spell* s)
{
    if (!s) return;
    // Global hooks (SpellCallback - 1 arg)
    EXEC_HOOKS(ASSpellHooks::instance()->GetHooks(t), _context->SetArgObject(0, s));

    // Per-spell hooks (can be SpellCallback or SpellHitCallback)
    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(s->GetSpellInfo()->Id, t))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;
        _context->SetArgObject(0, s);
        // If the function has 2 parameters, pass nullptr for the second one (target)
        if (f->GetParamCount() >= 2)
            _context->SetArgObject(1, nullptr);
        _context->Execute();
    }
}
void AngelScriptMgr::TriggerSpellCast(Spell* s) { TriggerSpellHook(SpellHookType::ON_CAST,s); }
void AngelScriptMgr::TriggerSpellHit(Spell* s, Unit* t)
{
    if (!s) return;
    // Global ON_HIT hooks (SpellCallback)
    for (auto& f : ASSpellHooks::instance()->GetHooks(SpellHookType::ON_HIT))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;
        _context->SetArgObject(0, s);
        if (t) _context->SetArgObject(1, t);
        _context->Execute();
    }
    // Per-spell-ID ON_HIT hooks (SpellHitCallback — void(Spell@, Unit@))
    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(s->GetSpellInfo()->Id, SpellHookType::ON_HIT))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;
        _context->SetArgObject(0, s);
        _context->SetArgObject(1, t);
        _context->Execute();
    }
}
void AngelScriptMgr::TriggerSpellEffectHit(Spell* s, uint8 i, Unit* t) { if(!s)return; for(auto& f:ASSpellHooks::instance()->GetHooks(SpellHookType::ON_EFFECT_HIT)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,s);_context->SetArgByte(1,i);if(t)_context->SetArgObject(2,t);_context->Execute();} }

void AngelScriptMgr::TriggerAuraCalcDamageAndHealing(AuraEffect const* aurEff, Unit* victim, int32& amount, int32& flatMod, float& pctMod)
{
    if (!aurEff || !victim) return;
    Aura* aur = aurEff->GetBase();
    if (!aur) return;

    SpellHookType hookType = (aurEff->GetAuraType() == SPELL_AURA_PERIODIC_HEAL) ? SpellHookType::ON_HEAL_CALC : SpellHookType::ON_DAMAGE_CALC;

    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(aur->GetId(), hookType))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;
        
        // Pass nullptr for spell since this is a periodic tick (no spell object)
        _context->SetArgObject(0, nullptr); 
        _context->SetArgByte(1, aurEff->GetEffIndex());
        _context->SetArgObject(2, victim);
        _context->SetArgAddress(3, &amount);
        _context->SetArgAddress(4, &flatMod);
        _context->SetArgAddress(5, &pctMod);
        _context->Execute();
    }
}

void AngelScriptMgr::TriggerAuraCalcAmount(AuraEffect const* aurEff, double& amount, bool& canBeRecalculated)
{
    if (!aurEff) return;
    Aura* aur = aurEff->GetBase();
    if (!aur) return;

    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(aur->GetId(), SpellHookType::ON_AURA_CALC_AMOUNT))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;

        _context->SetArgObject(0, (void*)aurEff);
        _context->SetArgAddress(1, &amount);
        _context->SetArgAddress(2, &canBeRecalculated);
        _context->Execute();
    }
}

void AngelScriptMgr::TriggerSpellAuraApply(Unit* target, AuraEffect const* aurEff)
{
    if (!target || !aurEff) return;

    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(aurEff->GetSpellInfo()->Id, SpellHookType::ON_AURA_APPLY))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;

        _context->SetArgObject(0, target);
        _context->SetArgObject(1, (void*)aurEff);
        _context->Execute();
    }
}

void AngelScriptMgr::TriggerSpellCalcDamage(Spell* s, uint8 i, Unit* t, int32& damage, int32& flatMod, float& pctMod)
{
    if (!s) return;
    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(s->GetSpellInfo()->Id, SpellHookType::ON_DAMAGE_CALC))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;
        _context->SetArgObject(0, s);
        _context->SetArgByte(1, i);
        _context->SetArgObject(2, t);
        _context->SetArgAddress(3, &damage);
        _context->SetArgAddress(4, &flatMod);
        _context->SetArgAddress(5, &pctMod);
        _context->Execute();
    }
}

void AngelScriptMgr::TriggerSpellCalcHealing(Spell* s, uint8 i, Unit* t, int32& healing, int32& flatMod, float& pctMod)
{
    if (!s) return;
    for (auto& f : ASSpellHooks::instance()->GetSpellHooks(s->GetSpellInfo()->Id, SpellHookType::ON_HEAL_CALC))
    {
        if (!_context) break;
        if (_context->Prepare(f) < 0) continue;
        _context->SetArgObject(0, s);
        _context->SetArgByte(1, i);
        _context->SetArgObject(2, t);
        _context->SetArgAddress(3, &healing);
        _context->SetArgAddress(4, &flatMod);
        _context->SetArgAddress(5, &pctMod);
        _context->Execute();
    }
}


bool AngelScriptMgr::TriggerSpellCheckCast(Spell* s)
{
    if(!s) return false;
    bool prevent=false;
    for(auto& f:ASSpellHooks::instance()->GetHooks(SpellHookType::ON_CHECK_CAST)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,s);if(_context->Execute()==asEXECUTION_FINISHED&&_context->GetReturnByte()!=0)prevent=true;}
    return prevent;
}

bool AngelScriptMgr::TriggerSpellEffect(Spell* s, uint8 idx, SpellEffectHandleMode mode)
{
    if(!s) return false;
    bool prevent=false;
    auto eh = ASSpellHooks::instance()->GetEffectHandlers(s->GetSpellInfo()->Id, idx);
    for(auto& f : eh){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,s);_context->SetArgByte(1,idx);_context->SetArgDWord(2,static_cast<uint32>(mode));if(_context->Execute()==asEXECUTION_FINISHED&&_context->GetReturnByte()!=0)prevent=true;}
    for(auto& f : ASSpellHooks::instance()->GetHooks(SpellHookType::ON_EFFECT_HIT)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,s);_context->SetArgByte(1,idx);_context->SetArgDWord(2,static_cast<uint32>(mode));if(_context->Execute()==asEXECUTION_FINISHED&&_context->GetReturnByte()!=0)prevent=true;}
    return prevent;
}

void AngelScriptMgr::TriggerQuestAccept(Player* p, Quest const* q) { if(!p||!q)return; EXEC_HOOKS(ASQuestHooks::instance()->GetHooks(QuestHookType::ON_ACCEPT), _context->SetArgObject(0,p); _context->SetArgObject(1,const_cast<Quest*>(q))); }
void AngelScriptMgr::TriggerQuestComplete(Player* p, Quest const* q) { if(!p||!q)return; EXEC_HOOKS(ASQuestHooks::instance()->GetHooks(QuestHookType::ON_COMPLETE), _context->SetArgObject(0,p); _context->SetArgObject(1,const_cast<Quest*>(q))); }
void AngelScriptMgr::TriggerQuestReward(Player* p, Quest const* q) { if(!p||!q)return; EXEC_HOOKS(ASQuestHooks::instance()->GetHooks(QuestHookType::ON_REWARD), _context->SetArgObject(0,p); _context->SetArgObject(1,const_cast<Quest*>(q))); }
void AngelScriptMgr::TriggerQuestStatusChange(Player* p, uint32 id) { if(!p)return; for(auto& f:ASQuestHooks::instance()->GetHooks(QuestHookType::ON_STATUS_CHANGE)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,p);_context->SetArgDWord(1,id);_context->Execute();} }
void AngelScriptMgr::TriggerQuestObjectiveUpdate(Player* p, uint32 qid, int32 oid, int32 oldA, int32 newA) { if(!p)return; for(auto& f:ASQuestHooks::instance()->GetHooks(QuestHookType::ON_STATUS_CHANGE)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,p);_context->SetArgDWord(1,qid);_context->SetArgDWord(2,static_cast<uint32>(oid));_context->SetArgDWord(3,static_cast<uint32>(oldA));_context->SetArgDWord(4,static_cast<uint32>(newA));_context->Execute();} }
void AngelScriptMgr::TriggerQuestAddKill(Player*,Quest*,uint32,uint32,uint32) {}
void AngelScriptMgr::TriggerQuestAddGather(Player*,Quest*,uint32,uint32,uint32) {}
void AngelScriptMgr::TriggerCraftStart(Player*,uint32,uint32) {}
void AngelScriptMgr::TriggerCraftComplete(Player*,uint32,uint32,bool) {}
void AngelScriptMgr::TriggerProfessionLearn(Player*,uint32) {}
void AngelScriptMgr::TriggerUnitDamageModified(Unit* t, Unit* a, int32& d) { if(!t||!a)return; for(auto& f:ASCreatureHooks::instance()->GetHooks(CreatureHookType::ON_DAMAGE)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,t);_context->SetArgObject(1,a);_context->SetArgDWord(2,static_cast<uint32>(d));_context->Execute();} }

bool AngelScriptMgr::TriggerPacketReceive(WorldSession* s, WorldPacket& p, uint32 op)
{
    if(!s) return false;
    auto* oh=ASPacketHooks::instance()->GetOpcodeHandler(op);
    if(oh && _context)
    {
        PacketData pd;
        pd.opcode = op;
        // Slice from the current read position — TC's WorldPacket may have already
        // consumed header bytes (rpos > 0). Using p.data() would give us those
        // header bytes and shift the entire payload, breaking bit/byte parsing.
        if (p.size() > p.rpos())
        {
            pd.data.assign(p.data() + p.rpos(), p.data() + p.size());
        }
        pd.size = static_cast<uint32>(pd.data.size());
        if(_context->Prepare(oh) >= 0)
        {
            _context->SetArgObject(0, s);
            _context->SetArgObject(1, &pd);
            if(_context->Execute() == asEXECUTION_FINISHED && _context->GetReturnByte() != 0)
                return ASPacketHooks::instance()->ShouldBlockOriginal(op);
        }
    }
    for(auto& f:ASPacketHooks::instance()->GetHooks(PacketHookType::ON_PACKET_RECEIVE)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,s);_context->SetArgObject(1,&p);_context->SetArgDWord(2,op);if(_context->Execute()==asEXECUTION_FINISHED&&_context->GetReturnByte()!=0)return true;}
    return false;
}

void AngelScriptMgr::TriggerPacketSend(WorldSession* s, WorldPacket& p, uint32 op)
{
    if(!s)return;
    for(auto& f:ASPacketHooks::instance()->GetHooks(PacketHookType::ON_PACKET_SEND)){if(!_context)break;if(_context->Prepare(f)<0)continue;_context->SetArgObject(0,s);_context->SetArgObject(1,&p);_context->SetArgDWord(2,op);_context->Execute();}
}

bool AngelScriptMgr::TriggerPacketReceive(WorldSession* s, uint16 op, WorldPacket& p) { return TriggerPacketReceive(s,p,static_cast<uint32>(op)); }
void AngelScriptMgr::SendPacketToPlayer(Player* p, uint16 op, const ByteBuffer& data) { if(!p||!p->GetSession())return; WorldPacket pkt(static_cast<uint16>(op)); pkt.append(data.data(),data.size()); p->GetSession()->SendPacket(&pkt); }

void AngelScriptMgr::TriggerCreatureGossipHello(Creature* c, Player* /*p*/) { TriggerCreatureHook(CreatureHookType::ON_GOSSIP_HELLO,c); }
void AngelScriptMgr::TriggerCreatureGossipSelect(Creature* c, Player* /*p*/, uint32 /*s*/, uint32 /*a*/) { TriggerCreatureHook(CreatureHookType::ON_GOSSIP_SELECT,c); }
void AngelScriptMgr::TriggerGameObjectGossipHello(GameObject* g, Player* /*p*/) { TriggerGameObjectHook(GameObjectHookType::ON_GOSSIP_HELLO,g); }
void AngelScriptMgr::TriggerGameObjectGossipSelect(GameObject* g, Player* /*p*/, uint32 /*s*/, uint32 /*a*/) { TriggerGameObjectHook(GameObjectHookType::ON_GOSSIP_SELECT,g); }

void AngelScriptMgr::TriggerCustomHook_SessionInitialized(WorldSession* session)
{
    if (!session) return;
    auto& hooks = _customHooks[static_cast<size_t>(CustomHookType::ON_SESSION_INITIALIZED)];
    for (auto& func : hooks)
    {
        if (!_context) break;
        int r = _context->Prepare(func);
        if (r < 0) continue;
        _context->SetArgObject(0, session);
        r = _context->Execute();
        if (r == asEXECUTION_EXCEPTION)
            TC_LOG_ERROR("server.angelscript", "[AS] SessionInitialized EXCEPTION: {} at {}:{}",
                _context->GetExceptionString(),
                _context->GetExceptionFunction() ? _context->GetExceptionFunction()->GetName() : "?",
                _context->GetExceptionLineNumber());
    }
}

} // namespace AngelScript
#endif // ANGELSCRIPT_INTEGRATION
