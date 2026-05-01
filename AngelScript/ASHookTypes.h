#ifndef ASHOOKTYPES_H
#define ASHOOKTYPES_H

#ifdef ANGELSCRIPT_INTEGRATION

namespace AngelScript
{
    // Hook types for various events
    enum class WorldHookType
    {
        ON_STARTUP,         // Server startup
        ON_SHUTDOWN,        // Server shutdown
        ON_UPDATE,          // World update tick
        ON_CONFIG_LOAD,     // Configuration loaded
        ON_CONSOLE_COMMAND, // Admin sends a command via console
        COUNT
    };

    enum class PlayerHookType
    {
        ON_LOGIN,           // Player logs in
        ON_LOGOUT,          // Player logs out
        ON_LEVEL_UP,        // Player levels up
        ON_KILL,            // Player kills something
        ON_DEATH,           // Player dies
        ON_CHAT,            // Player sends a chat message
        ON_KILL_PLAYER,     // Player kills another player
        ON_KILLED_BY_CREATURE, // Player killed by creature
        ON_DUEL_START,      // Duel starts
        ON_DUEL_END,        // Duel ends
        ON_MONEY_CHANGE,    // Money changes
        ON_GIVE_XP,         // XP given
        ON_REPUTATION_CHANGE, // Reputation changes
        ON_UPDATE_ZONE,     // Zone updates
        COUNT
    };

    enum class CreatureHookType
    {
        ON_SPAWN,           // Creature spawns
        ON_DEATH,           // Creature dies
        ON_ENTER_COMBAT,    // Creature enters combat
        ON_EXIT_COMBAT,     // Creature exits combat
        ON_DAMAGE,          // Creature takes damage
        ON_GOSSIP_HELLO,    // Gossip hello
        ON_GOSSIP_SELECT,   // Gossip option selected
        ON_KILL_PLAYER,     // Creature kills player
        COUNT
    };

    enum class GameObjectHookType
    {
        ON_SPAWN,           // GameObject spawns
        ON_USE,             // GameObject is used
        ON_GOSSIP_HELLO,    // Gossip hello
        ON_GOSSIP_SELECT,   // Gossip option selected
        COUNT
    };

    // Spell hook types
    enum class SpellHookType
    {
        ON_CAST,            // Spell is cast
        ON_HIT,             // Spell hits target
        ON_EFFECT_HIT,      // Spell effect hits
        ON_CHECK_CAST,      // Check if spell can be cast
        ON_DAMAGE_CALC,     // Calculate spell damage
        ON_HEAL_CALC,       // Calculate spell healing
        ON_BEFORE_CAST,     // Before spell cast starts
        ON_AFTER_CAST,      // After spell cast completes
        ON_CHANNEL_START,   // Channeling starts
        ON_CHANNEL_UPDATE,  // Channeling update
        ON_CHANNEL_END,     // Channeling ends
        ON_AURA_CALC_AMOUNT, // Calculate aura amount (points)
        COUNT
    };

    // Quest hook types
    enum class QuestHookType
    {
        ON_ACCEPT,          // Quest accepted
        ON_COMPLETE,        // Quest completed
        ON_REWARD,          // Quest rewarded
        ON_ADD_KILL,        // Quest kill credit added
        ON_ADD_GATHER,      // Quest gather item added
        ON_STATUS_CHANGE,   // Quest status changed
        ON_ABANDON,         // Quest abandoned
        ON_UPDATE,          // Quest update
        ON_QUEST_ACCEPT,    // Specific quest accepted
        ON_QUEST_COMPLETE,  // Specific quest completed
        ON_QUEST_REWARD,    // Specific quest rewarded
        ON_QUEST_STATUS_CHANGE, // Specific quest status changed
        ON_QUEST_OBJECTIVE_COMPLETE, // Specific objective completed
        ON_ITEM_USAGE,      // Item used for quest
        ON_SPELL_CAST,      // Spell cast for quest
        ON_GOSSIP_HELLO,    // Gossip hello for quest
        ON_GOSSIP_SELECT,   // Gossip option selected for quest
        COUNT
    };

    // Crafting hook types
    enum class CraftingHookType
    {
        ON_CRAFT_START,     // Crafting starts
        ON_CRAFT_COMPLETE,  // Crafting completes
        ON_CRAFT_CANCEL,    // Crafting cancelled
        ON_SKILL_CHECK,     // Skill check for crafting
        ON_PROFESSION_LEARN,// Profession learned
        ON_PROFESSION_UNLEARN,// Profession unlearned
        COUNT
    };

    // Packet hook types - for handling unimplemented opcodes
    enum class PacketHookType
    {
        ON_PACKET_RECEIVE,  // Any packet received (for unhandled opcodes)
        ON_PACKET_SEND,     // Any packet sent (before sending)
        COUNT
    };

    // Instance hook types
    enum class InstanceHookType
    {
        ON_INITIALIZE,      // Instance initialization
        ON_PLAYER_ENTER,    // Player enters instance
        ON_PLAYER_LEAVE,    // Player leaves instance
        ON_BOSS_KILLED,     // Boss killed in instance
        ON_RESET,           // Instance reset
        ON_COMPLETE,        // Instance completed
        ON_WIPE,            // Party wiped in instance
        ON_ENCOUNTER_START, // Encounter begins
        ON_ENCOUNTER_END,   // Encounter ends
        COUNT
    };

    // Battleground hook types
    enum class BattlegroundHookType
    {
        ON_START,           // Battleground starts
        ON_END,             // Battleground ends
        ON_PLAYER_JOIN,     // Player joins battleground
        ON_PLAYER_LEAVE,    // Player leaves battleground
        ON_FLAG_CAPTURE,    // Flag captured
        ON_FLAG_RETURN,     // Flag returned
        ON_POINT_CAPTURE,   // Control point captured
        ON_GATE_DESTROY,    // Gate destroyed
        ON_VEHICLE_USE,     // Vehicle used
        COUNT
    };

    // Arena hook types
    enum class ArenaHookType
    {
        ON_START,           // Arena match starts
        ON_END,             // Arena match ends
        ON_PLAYER_JOIN,     // Player joins arena
        ON_PLAYER_LEAVE,    // Player leaves arena
        ON_KILL,            // Player killed in arena
        ON_GATE_OPEN,       // Arena gates open
        COUNT
    };

    // Outdoor PvP hook types
    enum class OutdoorPvPHookType
    {
        ON_ZONE_CAPTURE,    // Zone captured
        ON_ZONE_LOST,       // Zone lost
        ON_TOWER_CAPTURE,   // Tower captured
        ON_TOWER_LOST,      // Tower lost
        ON_PLAYER_ENTER,    // Player enters PvP zone
        ON_PLAYER_LEAVE,    // Player leaves PvP zone
        ON_GRAVEYARD_CAPTURE, // Graveyard captured
        COUNT
    };

    // Map hook types (for world events)
    enum class MapHookType
    {
        ON_INITIALIZE,      // Map initialization
        ON_PLAYER_ENTER,    // Player enters map
        ON_PLAYER_LEAVE,    // Player leaves map
        ON_CREATURE_CREATE, // Creature created on map
        ON_GAMEOBJECT_CREATE, // GameObject created on map
        ON_WEATHER_CHANGE,  // Weather changes
        ON_TIME_CHANGE,     // Server time changes (day/night)
        COUNT
    };

    // Group/Guild hook types
    enum class GroupHookType
    {
        ON_CREATE,          // Group created
        ON_DISBAND,         // Group disbanded
        ON_MEMBER_ADD,      // Member added to group
        ON_MEMBER_REMOVE,   // Member removed from group
        ON_LEADER_CHANGE,   // Leader changed
        ON_LOOT_ROLL,       // Loot roll occurred
        COUNT
    };

    enum class GuildHookType
    {
        ON_CREATE,          // Guild created
        ON_DISBAND,         // Guild disbanded
        ON_MEMBER_ADD,      // Member added to guild
        ON_MEMBER_REMOVE,   // Member removed from guild
        ON_RANK_CHANGE,     // Member rank changed
        ON_MOTD_CHANGE,     // Message of the day changed
        ON_BANK_DEPOSIT,    // Item deposited to guild bank
        ON_BANK_WITHDRAW,   // Item withdrawn from guild bank
        COUNT
    };

    // Item hook types
    enum class ItemHookType
    {
        ON_USE,             // Item used
        ON_EQUIP,           // Item equipped
        ON_UNEQUIP,         // Item unequipped
        ON_LOOT,            // Item looted
        ON_TRADE,           // Item traded
        ON_DELETE,          // Item deleted
        ON_ENCHANT,         // Item enchanted
        ON_SOCKET_GEM,      // Gem socketed
        COUNT
    };

    // Auction House hook types
    enum class AuctionHookType
    {
        ON_CREATE,          // Auction created
        ON_BID,             // Bid placed
        ON_WIN,             // Auction won
        ON_EXPIRE,          // Auction expired
        ON_CANCEL,          // Auction cancelled
        COUNT
    };

    // ========================================================================
    // Custom Hook Points — for intercepting specific TC functions
    // To add a new hook point:
    //   1. Add enum here
    //   2. Add Trigger function in AngelScriptMgr.h/cpp
    //   3. Add Register function in AngelScriptMgr.h/cpp
    //   4. Patch the TC function to call the trigger
    //   5. Register the AS function in the appropriate API file
    // ========================================================================

    enum class CustomHookType
    {
        ON_SEND_PLAYER_CHOICE,      // Before SendPlayerChoice is sent (can modify/override)
        ON_GET_LOCKED_DUNGEONS,     // Before GetLockedDungeons returns (can modify lock list)
        ON_CAN_ENTER_INSTANCE,      // Before instance enter check
        ON_QUEST_CHOICE_REWARD,     // Before quest reward is given
        ON_LOOT_ITEM,              // Before loot item is generated
        ON_XP_CALCULATE,           // Before XP is calculated
        ON_REPUTATION_CALCULATE,   // Before reputation gain is calculated
        ON_GOLD_CALCULATE,         // Before gold reward is calculated
        ON_DAMAGE_CALCULATE,       // Before damage is calculated
        ON_HEAL_CALCULATE,         // Before healing is calculated
        ON_SPELL_LEARN,            // Before spell is learned
        ON_SPELL_UNLEARN,          // Before spell is unlearned
        ON_TALENT_CALCULATE,       // Before talent points are calculated
        ON_ITEM_LOOT_MODIFY,       // Modify item loot before it's given
        ON_AREATRIGGER,            // Area trigger entered/exited
        ON_CHAR_ENUM,              // After char enum packet is built; script can append warband groups and send
        COUNT
    };

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASHOOKTYPES_H
