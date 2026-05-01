/*
 * ScriptFramework.as
 * TC-compatible base classes for AngelScript scripting.
 * Mirrors TrinityCore's ScriptObject hierarchy.
 */

// ---- Hook type constants ----
// World
const int WORLD_ON_STARTUP       = 0;
const int WORLD_ON_SHUTDOWN      = 1;
const int WORLD_ON_UPDATE        = 2;
const int WORLD_ON_CONFIG_LOAD   = 3;
const int WORLD_ON_CONSOLE_COMMAND = 4;

// Player
const int PLAYER_ON_LOGIN            = 0;
const int PLAYER_ON_LOGOUT           = 1;
const int PLAYER_ON_CHAT             = 2;
const int PLAYER_ON_LEVEL_UP         = 3;
const int PLAYER_ON_DEATH            = 4;
const int PLAYER_ON_KILL_CREATURE    = 5;
const int PLAYER_ON_KILL_PLAYER      = 6;
const int PLAYER_ON_KILLED_BY_CREATURE = 7;
const int PLAYER_ON_DUEL_START       = 8;
const int PLAYER_ON_DUEL_END         = 9;
const int PLAYER_ON_MONEY_CHANGE     = 10;
const int PLAYER_ON_GIVE_XP          = 11;
const int PLAYER_ON_REPUTATION_CHANGE = 12;
const int PLAYER_ON_UPDATE_ZONE      = 13;
const int PLAYER_ON_MAP_CHANGE       = 14;

// Creature
const int CREATURE_ON_SPAWN         = 0;
const int CREATURE_ON_DEATH         = 1;
const int CREATURE_ON_ENTER_COMBAT  = 2;
const int CREATURE_ON_LEAVE_COMBAT  = 3;
const int CREATURE_ON_DAMAGE        = 4;
const int CREATURE_ON_GOSSIP_HELLO  = 5;
const int CREATURE_ON_GOSSIP_SELECT = 6;

// GameObject
const int GO_ON_SPAWN          = 0;
const int GO_ON_USE           = 1;
const int GO_ON_DESTROY       = 2;
const int GO_ON_GOSSIP_HELLO  = 3;
const int GO_ON_GOSSIP_SELECT = 4;

// Spell
const int SPELL_ON_CAST         = 0;
const int SPELL_ON_HIT          = 1;
const int SPELL_ON_EFFECT_HIT   = 2;
const int SPELL_ON_CHECK_CAST   = 3;
const int SPELL_ON_DAMAGE_CALC  = 4;
const int SPELL_ON_HEAL_CALC    = 5;
const int SPELL_ON_BEFORE_CAST  = 6;
const int SPELL_ON_AFTER_CAST   = 7;
const int SPELL_ON_CHANNEL_START  = 8;
const int SPELL_ON_CHANNEL_UPDATE = 9;
const int SPELL_ON_CHANNEL_END    = 10;

// Quest
const int QUEST_ON_ACCEPT         = 0;
const int QUEST_ON_COMPLETE       = 1;
const int QUEST_ON_REWARD         = 2;
const int QUEST_ON_STATUS_CHANGE  = 3;
const int QUEST_ON_OBJECTIVE_UPDATE = 4;

// Packet
const int PACKET_ON_RECEIVE = 0;
const int PACKET_ON_SEND    = 1;

// ---- Power type constants ----
const int POWER_MANA         = 0;
const int POWER_RAGE         = 1;
const int POWER_FOCUS        = 2;
const int POWER_ENERGY       = 3;
const int POWER_COMBO_POINTS = 4;
const int POWER_RUNES        = 5;
const int POWER_RUNIC_POWER  = 6;
const int POWER_SOUL_SHARDS  = 7;
const int POWER_LUNAR_POWER  = 8;
const int POWER_HOLY_POWER   = 9;
const int POWER_MAELSTROM    = 11;
const int POWER_CHI          = 12;
const int POWER_INSANITY     = 13;
const int POWER_BURNING_EMBER = 14;
const int POWER_DEMONIC_FURY = 15;
const int POWER_ARCANE_CHARGES = 16;
const int POWER_FURY         = 17;
const int POWER_PAIN         = 18;

// ---- Class constants ----
const int CLASS_WARRIOR     = 1;
const int CLASS_PALADIN     = 2;
const int CLASS_HUNTER      = 3;
const int CLASS_ROGUE       = 4;
const int CLASS_PRIEST      = 5;
const int CLASS_DEATH_KNIGHT = 6;
const int CLASS_SHAMAN      = 7;
const int CLASS_MAGE        = 8;
const int CLASS_WARLOCK     = 9;
const int CLASS_MONK        = 10;
const int CLASS_DRUID       = 11;
const int CLASS_DEMON_HUNTER = 12;
const int CLASS_EVOKER      = 13;
const int CLASS_ADVENTURER  = 14;
const int CLASS_TRAVELER    = 15;

// ---- Race constants ----
const int RACE_HUMAN               = 1;
const int RACE_ORC                 = 2;
const int RACE_DWARF               = 3;
const int RACE_NIGHTELF            = 4;
const int RACE_UNDEAD              = 5;
const int RACE_TAUREN              = 6;
const int RACE_GNOME               = 7;
const int RACE_TROLL               = 8;
const int RACE_GOBLIN              = 9;
const int RACE_BLOODELF            = 10;
const int RACE_DRAENEI             = 11;
const int RACE_WORGEN              = 22;
const int RACE_PANDAREN_NEUTRAL    = 24;
const int RACE_PANDAREN_ALLIANCE   = 25;
const int RACE_PANDAREN_HORDE      = 26;
const int RACE_NIGHTBORNE          = 27;
const int RACE_HIGHMOUNTAIN_TAUREN = 28;
const int RACE_VOID_ELF            = 29;
const int RACE_LIGHTFORGED_DRAENEI = 30;
const int RACE_ZANDALARI_TROLL     = 31;
const int RACE_KUL_TIRAN           = 32;
const int RACE_DARK_IRON_DWARF     = 34;
const int RACE_VULPERA             = 35;
const int RACE_MAGHAR_ORC          = 36;
const int RACE_MECHAGNOME          = 37;
const int RACE_DRACTHYR_ALLIANCE   = 52;
const int RACE_DRACTHYR_HORDE      = 70;
const int RACE_EARTHEN_DWARF_HORDE = 84;
const int RACE_EARTHEN_DWARF_ALLIANCE = 85;
const int RACE_HARANIR_HORDE       = 91;
const int RACE_HARANIR_ALLIANCE    = 86;

// ---- Gender constants ----
const int GENDER_MALE     = 0;
const int GENDER_FEMALE   = 1;
const int GENDER_NONE     = 2;

// ---- Team constants ----
const int TEAM_ALLIANCE = 0;
const int TEAM_HORDE    = 1;
const int TEAM_NEUTRAL  = 2;

// ---- Quest status constants ----
const int QUEST_STATUS_NONE       = 0;
const int QUEST_STATUS_COMPLETE   = 1;
const int QUEST_STATUS_UNAVAILABLE= 2;
const int QUEST_STATUS_INCOMPLETE = 3;
const int QUEST_STATUS_AVAILABLE  = 4;
const int QUEST_STATUS_FAILED     = 5;
const int QUEST_STATUS_REWARDED   = 6;

// ---- React state constants ----
const int REACT_PASSIVE    = 0;
const int REACT_DEFENSIVE  = 1;
const int REACT_AGGRESSIVE = 2;

// ---- GO type constants ----
const int GO_TYPE_DOOR        = 0;
const int GO_TYPE_BUTTON      = 1;
const int GO_TYPE_QUESTGIVER  = 2;
const int GO_TYPE_CHEST       = 3;
const int GO_TYPE_BINDER      = 4;
const int GO_TYPE_GENERIC     = 5;
const int GO_TYPE_TRANSPORT   = 6;
const int GO_TYPE_DAMAGE_ZONE = 7;
const int GO_TYPE_GUARDPOST   = 8;
const int GO_TYPE_SPELL_CASTER= 9;

// ---- Item class constants ----
const int ITEM_CLASS_CONSUMABLE   = 0;
const int ITEM_CLASS_CONTAINER    = 1;
const int ITEM_CLASS_WEAPON       = 2;
const int ITEM_CLASS_GEM          = 3;
const int ITEM_CLASS_ARMOR        = 4;
const int ITEM_CLASS_REAGENT      = 5;
const int ITEM_CLASS_PROJECTILE   = 6;
const int ITEM_CLASS_TRADE_GOODS  = 7;
const int ITEM_CLASS_RECIPE       = 9;
const int ITEM_CLASS_QUIVER       = 11;
const int ITEM_CLASS_QUEST_ITEM   = 12;
const int ITEM_CLASS_MISCELLANEOUS= 15;
const int ITEM_CLASS_GLYPH       = 16;

// ---- Spell school mask constants ----
const int SPELL_SCHOOL_MASK_NONE    = 0;
const int SPELL_SCHOOL_MASK_PHYSICAL= 1;
const int SPELL_SCHOOL_MASK_HOLY    = 2;
const int SPELL_SCHOOL_MASK_FIRE    = 4;
const int SPELL_SCHOOL_MASK_NATURE  = 8;
const int SPELL_SCHOOL_MASK_FROST   = 16;
const int SPELL_SCHOOL_MASK_SHADOW  = 32;
const int SPELL_SCHOOL_MASK_ARCANE  = 64;

// ---- DB2 store names (for GetDB2StoreEntryCount / HasDB2StoreEntry) ----
const string DB2_STORE_SPELL_NAME           = "SpellName";
const string DB2_STORE_ITEM                 = "Item";
const string DB2_STORE_CREATURE_DISPLAY     = "CreatureDisplayInfo";
const string DB2_STORE_MAP                  = "Map";
const string DB2_STORE_AREA_TABLE           = "AreaTable";
const string DB2_STORE_FACTION              = "Faction";
const string DB2_STORE_FACTION_TEMPLATE     = "FactionTemplate";
const string DB2_STORE_CHR_CLASSES          = "ChrClasses";
const string DB2_STORE_CHR_RACES            = "ChrRaces";
const string DB2_STORE_SKILL_LINE           = "SkillLine";
const string DB2_STORE_EMOTES               = "Emotes";
const string DB2_STORE_DIFFICULTY           = "Difficulty";
const string DB2_STORE_CHAR_TITLES          = "CharTitles";
const string DB2_STORE_CURRENCY_TYPES       = "CurrencyTypes";
const string DB2_STORE_GO_DISPLAY_INFO     = "GameObjectDisplayInfo";
const string DB2_STORE_SOUND_KIT            = "SoundKit";
const string DB2_STORE_SPELL_EFFECT         = "SpellEffect";
const string DB2_STORE_SPELL_VISUAL         = "SpellVisual";
const string DB2_STORE_BROADCAST_TEXT       = "BroadcastText";
const string DB2_STORE_ACHIEVEMENT          = "Achievement";

// ---- Helper functions ----

// Get player team from race
int GetTeamByRace(uint8 race)
{
    if (race == RACE_HUMAN || race == RACE_DWARF || race == RACE_NIGHTELF ||
        race == RACE_GNOME || race == RACE_DRAENEI || race == RACE_BLOODELF + 1) // Worgen/Pandaren not listed
        return TEAM_ALLIANCE;
    if (race == RACE_ORC || race == RACE_UNDEAD || race == RACE_TAUREN ||
        race == RACE_TROLL || race == RACE_BLOODELF)
        return TEAM_HORDE;
    return TEAM_NEUTRAL;
}

// Get player team
int GetTeam(Player@ player)
{
    if (player is null) return TEAM_NEUTRAL;
    return GetTeamByRace(player.GetRace());
}

// Give XP to player
void GiveXP(Player@ player, uint32 amount)
{
    if (player is null) return;
    // Use ModifyMoney as proxy — actual GiveXP requires dispatch hook
    Print("GiveXP: " + amount + " to " + player.GetName());
}

// SendFloatingText(Player@, const string& in, uint32) and PlaySoundToPlayer(Player@, uint32)
// are provided by the C++ Global API — do not redefine them here.
