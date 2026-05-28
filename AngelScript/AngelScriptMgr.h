/*
 * This file is part of the TrinityCore Project with AngelScript integration.
 *
 * AngelScript integration allows runtime scripting without server restart.
 * API registration is delegated to API/ directory files.
 * Core hooks are routed through Dispatch/ layer via TC's ScriptMgr.
 */

#ifndef ANGELSCRIPTMGR_H
#define ANGELSCRIPTMGR_H

// Forward declarations for AngelScript
class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;
class Unit;
class Spell;
class Aura;
class AuraEffect;
class Player;
class GameObject;
class Quest;
class asIScriptFunction;
class CScriptBuilder;

namespace WorldPackets::Character
{
    class EnumCharactersResult;
}

#ifdef ANGELSCRIPT_INTEGRATION

#include "Common.h"
#include "ObjectGuid.h"
#include "ASPacketData.h"
#include "ASHookTypes.h"
#include "Hooks/ASWorldHooks.h"
#include "Hooks/ASPlayerHooks.h"
#include "Hooks/ASCreatureHooks.h"
#include "Hooks/ASGameObjectHooks.h"
#include "Hooks/ASSpellHooks.h"
#include "Hooks/ASQuestHooks.h"
#include "Hooks/ASCraftingHooks.h"
#include "Hooks/ASPacketHooks.h"
#include "Hooks/ASInstanceHooks.h"
#include "Hooks/ASBattlegroundHooks.h"
#include "ASScriptAttributes.h"
#include "ASGenericDB2Loader.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class asIScriptFunction;
class CScriptBuilder;

class Player;
class Creature;
class CreatureAI;
class GameObject;
class GameObjectAI;
class Map;
class InstanceMap;
class InstanceScript;
class WorldSession;
class Unit;
class Spell;
class Aura;
class AuraEffect;
class SpellInfo;
class Quest;
class Item;
class WorldPacket;
class ByteBuffer;

// DuelCompleteType is an enum defined in SharedDefinitions.h
#include "SharedDefines.h"

namespace AngelScript
{
    // Script callback storage
    using ScriptCallback = std::function<void()>;

    // AngelScript Manager - Singleton
    // Slimmed: engine lifecycle, script loading, hook dispatch only.
    // API registration delegated to API/ directory files.
    // Core integration routed through Dispatch/ layer.
    class TC_GAME_API AngelScriptMgr
    {
    public:
        static AngelScriptMgr* instance();

        // Initialize/Shutdown
        void Initialize();
        void Shutdown();
        bool InitializeAndReturn(); // Returns true on success
        void ReloadScripts();
        void ReloadScript(const std::string& scriptName);
        void GetStatus(std::string& output);

        // Script loading
        bool LoadAllScripts();
        void UnloadAllScripts();
        bool CompileScript(const std::string& filename, const std::string& moduleName);

        // Hook registration for scripts
        void RegisterWorldScript(WorldHookType type, asIScriptFunction* func);
        void RegisterPlayerScript(PlayerHookType type, asIScriptFunction* func);
        void RegisterCreatureScript(CreatureHookType type, asIScriptFunction* func);
        void RegisterGameObjectScript(GameObjectHookType type, asIScriptFunction* func);
        void RegisterSpellHook(SpellHookType type, asIScriptFunction* func);
        void RegisterSpellHook(uint32 spellId, SpellHookType type, asIScriptFunction* func);  // Spell-specific
        void RegisterSpellEffectHandler(uint32 spellId, uint8 effIndex, asIScriptFunction* func);
        void RegisterQuestScript(QuestHookType type, asIScriptFunction* func);
        void RegisterCraftingScript(CraftingHookType type, asIScriptFunction* func);
        void RegisterPacketScript(PacketHookType type, asIScriptFunction* func);
        
        // Enhanced hook registration
        void RegisterInstanceScript(InstanceHookType type, asIScriptFunction* func);
        void RegisterInstanceScript(uint32 mapId, ASInstanceScript* script);
        void RegisterBattlegroundScript(BattlegroundHookType type, asIScriptFunction* func);
        void RegisterBattlegroundScript(uint32 bgTypeId, ASBattlegroundScript* script);
        void RegisterArenaScript(ArenaHookType type, asIScriptFunction* func);
        void RegisterArenaScript(uint32 arenaTypeId, ASArenaScript* script);
        void RegisterMapScript(MapHookType type, asIScriptFunction* func);
        void RegisterGroupScript(GroupHookType type, asIScriptFunction* func);
        void RegisterGuildScript(GuildHookType type, asIScriptFunction* func);
        void RegisterItemScript(ItemHookType type, asIScriptFunction* func);
        void RegisterAuctionScript(AuctionHookType type, asIScriptFunction* func);

        // Custom hook points — for intercepting specific TC functions
        void RegisterCustomHook(CustomHookType type, asIScriptFunction* func);

        // Per-entry creature/GO AI registration
        // Undef TC macros that conflict with these method names
#undef RegisterCreatureAI
#undef RegisterGameObjectAI
        void RegisterCreatureAI(uint32 entry, asIScriptFunction* factory);
        void RegisterGameObjectAI(uint32 entry, asIScriptFunction* factory);

        // Hook triggering — called from Dispatch/ layer
        void TriggerWorldHook(WorldHookType type);
        void TriggerWorldUpdate(uint32 diff);
        void TriggerConsoleCommand(std::string& command);
        void TriggerPlayerHook(PlayerHookType type, Player* player);
        void TriggerPlayerHook(PlayerHookType type, Player* player, Player* other); // ON_KILL_PLAYER, ON_DUEL_*
        void TriggerPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg);
        void TriggerPlayerKill(Player* player, Unit* victim);
        void TriggerPlayerDeath(Player* player, Unit* killer);
        void TriggerPlayerLevelUp(Player* player, uint8 newLevel);
        void TriggerPlayerDuelStart(Player* player1, Player* player2);
        void TriggerPlayerDuelEnd(Player* winner, Player* loser, DuelCompleteType type);
        void TriggerPlayerMoneyChanged(Player* player, int64& amount);
        void TriggerPlayerGiveXP(Player* player, uint32& amount, Unit* victim);
        void TriggerPlayerReputationChange(Player* player, uint32 factionId, int32& standing, bool incremental);
        void TriggerPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea);
        void TriggerCreatureHook(CreatureHookType type, Creature* creature);
        void TriggerCreatureKillPlayer(Creature* creature, Player* player);
        void TriggerGameObjectHook(GameObjectHookType type, GameObject* go);
        void TriggerGameObjectUse(GameObject* go, Player* player);
        
        // Spell hooks
        void TriggerSpellHook(SpellHookType type, Spell* spell);
        void TriggerSpellCast(Spell* spell);
        void TriggerSpellHit(Spell* spell, Unit* target);
        void TriggerSpellEffectHit(Spell* spell, uint8 effIndex, Unit* target);
        void TriggerSpellCalcDamage(Spell* spell, uint8 effIndex, Unit* target, int32& damage, int32& flatMod, float& pctMod);
        void TriggerSpellCalcHealing(Spell* spell, uint8 effIndex, Unit* target, int32& healing, int32& flatMod, float& pctMod);
        void TriggerAuraCalcDamageAndHealing(AuraEffect const* aurEff, Unit* victim, int32& amount, int32& flatMod, float& pctMod);
        void TriggerAuraCalcAmount(AuraEffect const* aurEff, double& amount, bool& canBeRecalculated);
        void TriggerSpellAuraApply(Unit* target, AuraEffect const* aurEff);
        bool TriggerSpellCheckCast(Spell* spell);
        bool TriggerSpellEffect(Spell* spell, uint8 effIndex, SpellEffectHandleMode mode);
        
        // Quest hooks
        void TriggerQuestAccept(Player* player, Quest const* quest);
        void TriggerQuestComplete(Player* player, Quest const* quest);
        void TriggerQuestReward(Player* player, Quest const* quest);
        void TriggerQuestStatusChange(Player* player, uint32 questId);
        void TriggerQuestObjectiveUpdate(Player* player, uint32 questId, int32 objectiveId, int32 oldAmount, int32 newAmount);
        void TriggerQuestAddKill(Player* player, Quest* quest, uint32 entry, uint32 count, uint32 maxCount);
        void TriggerQuestAddGather(Player* player, Quest* quest, uint32 entry, uint32 count, uint32 maxCount);
        
        // Crafting hooks
        void TriggerCraftStart(Player* player, uint32 professionId, uint32 recipeId);
        void TriggerCraftComplete(Player* player, uint32 professionId, uint32 recipeId, bool success);
        void TriggerProfessionLearn(Player* player, uint32 professionId);
        
        // Unit hooks
        void TriggerUnitDamageModified(Unit* target, Unit* attacker, int32& damage);
        
        // Packet hooks - returns true if packet was handled by AngelScript
        bool TriggerPacketReceive(WorldSession* session, WorldPacket& packet, uint32 opcode);
        void TriggerPacketSend(WorldSession* session, WorldPacket& packet, uint32 opcode);

        // Gossip hooks
        void TriggerCreatureGossipHello(Creature* creature, Player* player);
        void TriggerCreatureGossipSelect(Creature* creature, Player* player, uint32 sender, uint32 action);
        void TriggerGameObjectGossipHello(GameObject* go, Player* player);
        void TriggerGameObjectGossipSelect(GameObject* go, Player* player, uint32 sender, uint32 action);

        // Custom hook points — returns true if hook handled/overrode the event
        bool TriggerCustomHook_SendPlayerChoice(Player* player, int32 choiceId);
        bool TriggerCustomHook_GetLockedDungeons(Player* player, std::vector<uint32>& lockedDungeons);
        bool TriggerCustomHook_CharEnum(WorldSession* session, WorldPackets::Character::EnumCharactersResult& enumResult);
        bool TriggerCustomHook_AuthResponse(WorldSession* session, uint32& currencyID);
        bool TriggerCustomHook_FeatureSystemStatusGlueScreen(WorldSession* session, bool& bpayStoreAvailable, int32& activeBoostType, bool& commerceServerEnabled, uint32& commercePricePollTimeSeconds, int32& contentSetID);
        bool TriggerCustomHook_FeatureSystemStatus(WorldSession* session, bool& bpayStoreAvailable, bool& commerceServerEnabled, uint32& commercePricePollTimeSeconds);
        void TriggerCustomHook_SessionInitialized(WorldSession* session);

        // Per-entry AI dispatch — called from Dispatch/ CreatureScript/GameObjectScript
        CreatureAI* GetCreatureAI(Creature* creature);
        GameObjectAI* GetGameObjectAI(GameObject* go);
        InstanceScript* GetInstanceScript(InstanceMap* map);

        // Utility
        asIScriptEngine* GetEngine() const { return _scriptEngine; }
        bool IsEnabled() const { return _enabled; }

        // Script file management
        void SetScriptPath(const std::string& path) { _scriptPath = path; }
        std::string GetScriptPath() const { return _scriptPath; }

    private:
        AngelScriptMgr();
        ~AngelScriptMgr();

        bool SetupEngine();
        bool LoadScripts();
        void UnloadScripts();
        void RegisterStandardAddons();
        void RegisterTrinityCoreAPI();

        // API registration delegated to API/ directory
        // These are now thin wrappers calling the API/ functions
        void RegisterGlobalFunctions();
        void RegisterPlayerAPI();
        void RegisterCreatureAPI();
        void RegisterGameObjectAPI();
        void RegisterUnitAPI();
        void RegisterSpellAPI();
        void RegisterAuraAPI();
        void RegisterItemAPI();
        void RegisterCraftingAPI();
        void RegisterPacketAPI();
        void RegisterQuestAPI();
        void RegisterMathAPI();
        void RegisterStringAPI();
        void RegisterDB2API();
        void RegisterDynamicDB2API();
        void InitializeDB2Loader();
        void RegisterSharedDataAPI();
        void RegisterScriptClassesAPI();
        void RegisterDatabaseAPI();
        void RegisterWorldAPI();
        void RegisterUpdateFieldAPI();
        void RegisterGossipAPI();
        void RegisterEnhancedPacketAPI();
        void RegisterScriptAttributesAPI();
        void RegisterSpawnAPI();
        void RegisterInstanceAPI();
        void RegisterBattlegroundAPI();
        void RegisterArenaAPI();
        void RegisterMapAPI();
        void RegisterGroupGuildAPI();
        void RegisterItemAuctionAPI();

        // Packet handling
        bool TriggerPacketReceive(WorldSession* session, uint16 opcode, WorldPacket& packet);
        void SendPacketToPlayer(Player* player, uint16 opcode, const ByteBuffer& data);

        // Helper to execute a script function safely
        bool ExecuteScriptFunction(asIScriptFunction* func);

        asIScriptEngine* _scriptEngine;
        CScriptBuilder* _scriptBuilder;
        asIScriptContext* _context;

        std::atomic<bool> _enabled;
        std::string _scriptPath;
        std::string _scriptCachePath;

        // Per-entry AI factories
        std::unordered_map<uint32, asIScriptFunction*> _creatureAIFactories;
        std::unordered_map<uint32, asIScriptFunction*> _gameObjectAIFactories;

        // Per-entry instance scripts
        std::unordered_map<uint32, ASInstanceScript*> _instanceScripts;

        // Allow hook managers to access private members
        friend class ASPacketHooks;
        friend class ASInstanceHooks;
        friend class ASBattlegroundHooks;

        // Module cache for hot-reloading
        std::unordered_map<std::string, asIScriptModule*> _modules;

        // Custom hook storage (indexed by CustomHookType)
        std::array<std::vector<asIScriptFunction*>, static_cast<size_t>(CustomHookType::COUNT)> _customHooks;

        // Attribute parser for enhanced script registration
        ASScriptAttributeParser _attributeParser;
    };

#define sAngelScriptMgr AngelScript::AngelScriptMgr::instance()

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION

#endif // ANGELSCRIPTMGR_H
