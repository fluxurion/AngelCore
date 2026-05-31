# AngelScript for AngelCore

AngelScript integration for AngelCore — write game scripts in a C++-like scripting language
with hot-reload support, independent database, and in-game shop system.

## Quick Start

1. Build the server — AngelScript is enabled by default
2. `.as` scripts in the `angelscripts/` folder are auto-loaded on startup
3. Use `#rel as` in game or console to reload all scripts

## Architecture

```
angelscripts/
├── Config.as                  — Global configuration (BattlePay, AngelDB, catch-up settings)
├── ScriptFramework.as         — Constants, hook types, helper functions
├── Common.as                  — Type aliases, math constants, utilities
├── AngelDB/                   — SQL update system
│   ├── pending/               — .sql files to execute on startup/reload
│   ├── applied/               — executed files
│   └── README.md              — AngelDB documentation
├── battlepay/                 — In-game shop system
│   ├── BattlePay.as           — Main module entry point
│   ├── BattlePayEnums.as      — Result codes and constants
│   ├── BattlePayData.as       — Product data loading from AngelDB
│   ├── BattlePayDispatch.as   — Purchase dispatch logic
│   ├── BattlePayPackets.as    — Packet builders (SMSG handlers)
│   ├── BattlePayStubs.as      — Delivery functions (mounts, toys, spells, items)
│   ├── BattlePayHooks.as      — Auth response, feature status, session init hooks
│   ├── BattlePayCommands.as   — #bpay chat commands
│   ├── BattlePayDelivery.as   — Mail-based item delivery
│   └── CharacterServices.as   — Name/race/faction change services
├── characters/                — Character systems
│   ├── CharacterCatchUp.as    — Gear catch-up system
│   └── CharacterUpgrade.as    — Character boost packages
├── warband/                   — Warband groups system
│   ├── warband_groups.as      — Warband group CRUD and char enum integration
│   └── warband_scene_unlock.as — Warband scene unlock management
└── examples/                  — Example scripts by category
```

## Commands

All AngelScript commands use `#` prefix and work from both in-game chat and server console:

| Command | Description |
|---|---|
| `#rel as` | Reload all AngelScript scripts |
| `#bpay credits` | View your BattlePay credit balance |
| `#bpay addcredits <amount> [player]` | Add credits to yourself or another player |
| `#bpay product <id> [player]` | Deliver a product to yourself or another player |
| `#bpay service <namechange\|factionchange\|racechange>` | Apply a character service |
| `#bpay reload` | Reload BattlePay product data from database |
| `#bpay info` | Show loaded product count and shop status |
| `#bpay gear [level] [player]` | Request gear catch-up |
| `#bpay upgrade [player]` | Apply character upgrade package |

## AngelDB — Independent Database

All script data lives in a separate MySQL database (`angelcore_scripts` by default).
Credentials are read from `worldserver.conf` — the same MySQL server as TC's databases.

### SQL Update System

Place `.sql` files in `AngelDB/pending/` — they run automatically on startup/reload:

```
AngelDB/
├── pending/     — 0001_battlepay_world.sql, 0002_battlepay_transactions.sql, ...
└── applied/     — successfully executed files are moved here
```

- Files execute in alphabetical order
- Successful → moved to `applied/`
- Failed → stays in `pending/` (check server logs)
- `CREATE TABLE IF NOT EXISTS` and `INSERT IGNORE` are idempotent — safe to re-run

### AngelScript API

```angelscript
AngelDB_Query("SELECT ...")         // Returns AngelDBResult (value type)
AngelDB_Execute("INSERT ...")       // Returns bool
AngelDB_EscapeString("input")       // Returns escaped string
AngelDB_IsConnected()               // Returns bool
AngelDB_GetLastError()              // Returns error string
AngelDB_RunPendingUpdates("path")   // Manual trigger, returns count
```

### AngelDBResult (value type, no null check needed)

```angelscript
AngelDBResult r = AngelDB_Query("SELECT id, name FROM spawns WHERE map = 0");
if (r.GetRowCount() > 0)              // Check if rows exist
{
    while (r.NextRow())               // Iterate rows
    {
        uint32 id = r.GetUInt32(0);    // Column 0 as uint32
        string name = r.GetString(1);   // Column 1 as string
        float x = r.GetFloat(2);        // Column 2 as float
    }
}
```

## BattlePay Shop System

Fully functional in-game shop with product listings, purchase flow, and delivery.

### Tables (in AngelDB)

| Table | Purpose |
|---|---|
| `battlepay_display_infos` | Visual display metadata (icons, descriptions, previews) |
| `battlepay_product_infos` | Product listings (prices, flags, deliverable IDs) |
| `battlepay_product_datas` | Deliverable products (items, mounts, pets, services) |
| `battlepay_shop_datas` | Shop layout (grouping, ordering, flags) |
| `battlepay_groups` | Category groups |
| `battlepay_credits` | Per-account credit balance |
| `battlepay_purchases` | Purchase history |
| `battlepay_distributions` | Offline delivery queue |
| `battlepay_pending_rewards` | Pending mail/SQL rewards |
| `character_catchup_requests` | Gear catch-up queue |
| `battlepay_guild_services` | Guild service tracking |

### Configuration (`Config.as`)

```angelscript
bool   CONFIG_BPAY_STORE_ENABLED = true;       // Enable shop
uint32 CONFIG_BPAY_STORE_CURRENCY = 3;          // Currency ID (3 = BattleCoins)
string CONFIG_ANGELDB_DATABASE = "angelcore_scripts";  // AngelDB name
string CONFIG_ANGELDB_UPDATES_DIR = "AngelDB";  // Updates folder
```

## Warband Groups System

Character grouping system for the Warband character selection screen.

### Tables (in AngelDB)

| Table | Purpose |
|---|---|
| `warband_groups` | Group definitions per account (name, scene, order) |
| `warband_group_members` | Character-to-group assignments |

Scripts auto-create a "Favorites" group per account and integrate with the character
enumeration packet to display groups on the character selection screen.

## Hook Registration

```angelscript
#include "ScriptFramework.as"

void OnLogin(Player@ player) { SendSystemMessage(player, "Welcome!"); }
void main() { RegisterPlayerScript(PLAYER_ON_LOGIN, @OnLogin); }
```

### Available Hook Types

| Category | Constants | Registration |
|---|---|---|
| World | `WORLD_ON_STARTUP`, `WORLD_ON_SHUTDOWN`, `WORLD_ON_UPDATE`, `WORLD_ON_CONFIG_LOAD` | `RegisterWorldScript()` |
| Console | `WORLD_ON_CONSOLE_COMMAND` | `RegisterConsoleCommandHook()` |
| Player | `PLAYER_ON_LOGIN`, `PLAYER_ON_LOGOUT`, `PLAYER_ON_CHAT`, `PLAYER_ON_LEVEL_UP`, `PLAYER_ON_DEATH`, `PLAYER_ON_KILL_CREATURE`, `PLAYER_ON_KILL_PLAYER`, `PLAYER_ON_DUEL_START`, `PLAYER_ON_DUEL_END`, `PLAYER_ON_MONEY_CHANGE`, `PLAYER_ON_GIVE_XP`, `PLAYER_ON_REPUTATION_CHANGE`, `PLAYER_ON_UPDATE_ZONE`, `PLAYER_ON_MAP_CHANGE` | `RegisterPlayerScript()` |
| Creature | `CREATURE_ON_SPAWN`, `CREATURE_ON_DEATH`, `CREATURE_ON_ENTER_COMBAT`, `CREATURE_ON_LEAVE_COMBAT`, `CREATURE_ON_DAMAGE`, `CREATURE_ON_GOSSIP_HELLO`, `CREATURE_ON_GOSSIP_SELECT` | `RegisterCreatureScript()` |
| GameObject | `GO_ON_SPAWN`, `GO_ON_USE`, `GO_ON_DESTROY`, `GO_ON_GOSSIP_HELLO`, `GO_ON_GOSSIP_SELECT` | `RegisterGameObjectScript()` |
| Spell | `SPELL_ON_CAST`, `SPELL_ON_HIT`, `SPELL_ON_EFFECT_HIT`, `SPELL_ON_CHECK_CAST` | `RegisterSpellHook()` |
| Quest | `QUEST_ON_ACCEPT`, `QUEST_ON_COMPLETE`, `QUEST_ON_REWARD` | `RegisterQuestScript()` |
| Packet | `PACKET_ON_RECEIVE`, `PACKET_ON_SEND` | `RegisterPacketScript()` |

## API Reference

### Player (45+ methods)
```
GetName, GetLevel, GetClass, GetRace, GetGender
IsAlive, IsDead, IsInCombat, IsOnline, IsAFK, IsDND, IsGM, SetGM, IsMounted
GetMapId, GetZoneId, GetAreaId, GetPositionXYZO
TeleportTo, GetHealth, GetMaxHealth, GetHealthPct
GetMoney, ModifyMoney, GetItemCount, HasItemCount, AddItem
GetPower, SetPower, GetQuestStatus, CompleteQuest, FailQuest
GetReputation, SetReputation, GetGuildId, GetAccountId, GetAccountName
SendNotification, HasAura, AddAura, RemoveAura, CastSpell, Kill, GiveXP
```

### Creature (55+ methods)
```
GetName, GetEntry, GetGUID, GetLevel, IsAlive, IsDead, IsInCombat
SetInCombatWith, GetHealth, SetHealth, SetFullHealth, GetHealthPct
HasAura, GetReactState, SetReactState, HasQuest
Respawn, DespawnOrUnsummon, GetFaction, IsFriendlyTo, IsHostileTo
GetMapId, GetPositionXYZO, AttackStart, CastSpell, AddAura, Kill
```

### Global Functions
```
Print(msg)                              — Log to console
SendSystemMessage(player, msg)          — System message to player
FindPlayerByName(name)                  — Find online player
FindPlayerByGUID(guid)                  — Find player by GUID
GetGameTime()                           — Current game time
```

### DB2 Data Access
```angelscript
GetSpellName(133)           // "Fireball"
GetItemName(17349)          // Item name
GetMapName(0)               // "Eastern Kingdoms"
GetClassName(1)             // "Warrior"
GetRaceName(1)              // "Human"
HasDB2StoreEntry("Item", 17349)
```

### Database API (for TC databases — use AngelDB for script data)
```angelscript
CharacterQuery("SELECT ...")
CharacterExecute("INSERT ...")
WorldQuery("SELECT ...")
WorldExecute("UPDATE ...")
```

### Spawn API
```angelscript
SpawnCreature(entry, map, x, y, z, o, phaseId)
SpawnGameObject(entry, map, x, y, z, o, phaseId)
ConfigureCreature(creature, level, faction, npcFlags, gossip, equip, react)
DespawnCreature(creature)
```

## Defense Mechanism

Failed scripts are **rejected** — they cannot crash or corrupt the server:

```
[AS-ERR] script.as(15): Function 'MissingFunc' not found
[DEFENSE] Script 'script.as' failed to compile (1 errors)
```

- Syntax errors reported with file name + line number
- Type mismatches caught at compile time
- Runtime exceptions logged with stack trace
- Failed module is discarded, existing hooks preserved

## Notes

- Scripts run on the main server thread — avoid blocking operations
- AngelDB queries use a separate MySQL connection (doesn't block TC's pool)
- `#` prefix commands are intercepted before chat broadcast
- `AngelDBResult` is a value type — check `GetRowCount() > 0` instead of `is null`
- DB2 data is shared from TrinityCore — always up-to-date, no custom loading needed
