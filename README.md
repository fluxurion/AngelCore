# AngelCore — TrinityCore + AngelScript Integration

AngelCore is a custom World of Warcraft server project based on **TrinityCore**
with a deep **AngelScript integration** — a fully scriptable game server where most custom
game logic can be written in AngelScript (`.as`) files with hot-reload support.

## What is AngelScript?

AngelScript is a C++-like scripting language embedded directly into the server. It allows
writing game logic without modifying or recompiling C++ source code. Scripts are compiled at
load time, run on the server's main thread, and can be reloaded at any time without restarting
the server.

## Why AngelScript?

- **No C++ recompiles** — iterate on game logic instantly
- **Hot-reload** — `#rel as` reloads all scripts in seconds, preserving server uptime
- **Isolation** — failed scripts are rejected with detailed error messages, never crash the server
- **Full API access** — players, creatures, gameobjects, spells, quests, packets, DB2 data
- **Independent database** — AngelDB provides a separate MySQL database for script data

### Core Philosophy: Minimal Merge Conflicts

The entire purpose of the AngelScript integration is to **keep AngelCore custom work
completely separated from TrinityCore upstream code**. When TrinityCore releases updates:

- **Only hook insertion points** in the C++ source can cause merge conflicts — and those
  are minimal (~20 locations across the entire codebase)
- **All game logic** lives in `.as` files — never touches TC source
- **All script data** lives in AngelDB — never pollutes TC's auth/characters/world databases
- **All SQL migrations** run through the AngelDB update system — no TC SQL directory changes

This means you can pull upstream TrinityCore changes with confidence, knowing that the
merge surface is tiny and well-defined.

## Architecture

```
AngelCore/
├── AngelScript/              — C++ integration layer
│   ├── API/                  — Per-type AngelScript bindings
│   ├── Hooks/                — Hook managers (player, creature, spell, packet, ...)
│   ├── Dispatch/             — TC ScriptObject bridge
│   ├── SDK/                  — AngelScript compiler & VM (v2.38.0)
│   ├── AngelScriptMgr.cpp    — Engine lifecycle, script loading, hook dispatch
│   └── angelscripts/         — All .as script files live here
├── src/server/               — TrinityCore source (with hook insertion points)
└── sql/custom/               — Legacy SQL files (migrated to AngelDB)
```

## Quick Start

```bash
# Build with AngelScript enabled (default)
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo

# Scripts are copied to the binary output directory automatically
# Start the server and use #rel as to reload
```

## In-Game Commands

All AngelScript commands use `#` prefix:

| Command | Description |
|---|---|
| `#rel as` | Reload all AngelScript scripts |
| `#bpay credits` | View BattlePay balance |
| `#bpay addcredits <amount>` | Add BattlePay credits |
| `#bpay product <id>` | Purchase a product directly |
| `#bpay service <type>` | Apply character service |
| `#bpay reload` | Reload BattlePay product data |
| `#bpay info` | Show BattlePay status |
| `#bpay gear` | Request gear catch-up |
| `#bpay upgrade` | Apply character upgrade package |

## AngelDB — Independent MySQL Database

AngelDB is a standalone MySQL database (`angelcore_db`) used for all custom script data.
It reuses the same MySQL server as the world/character databases but keeps script data
completely separate from TrinityCore's tables.

**Key features:**
- **Auto-created** on first startup — no manual setup needed
- **Auto-updates** — `.sql` files in `angelscripts/AngelDB/pending/` run automatically
- **Credentials from `worldserver.conf`** — same host/port/user/pass as the world database
- **Separate thread** — never blocks TC's main database pool

```
angelscripts/AngelDB/
├── pending/     — Drop .sql files here, they run on next startup/reload
├── applied/     — Successfully executed files are moved here
└── README.md    — Full documentation
```

## Hook System

Scripts register for game events via hooks. Over 50 hook types across 10 categories:

```angelscript
void OnLogin(Player@ player) { /* ... */ }
void main() { RegisterPlayerScript(PLAYER_ON_LOGIN, @OnLogin); }
```

| Category | Examples |
|---|---|
| Player | Login, logout, chat, level up, death, duel, reputation |
| Creature | Spawn, death, combat, damage, gossip |
| GameObject | Spawn, use, destroy, gossip |
| Spell | Cast, hit, effect, damage calc, aura apply |
| Quest | Accept, complete, reward, status change |
| Packet | Receive, send |
| World | Startup, shutdown, update tick, console command |
| Instance | Enter, leave, boss kill |
| Battleground | Start, join, leave, objective |
| Crafting | Start, complete, profession learn |

## API Coverage

| API | Method Count | Highlights |
|---|---|---|
| Player | 45+ | TeleportTo, AddItem, ModifyMoney, CastSpell, SetReputation |
| Creature | 55+ | AttackStart, DespawnOrUnsummon, SetReactState, GetLootState |
| GameObject | 35+ | SetGoState, Use, Respawn, SetRespawnTime |
| Unit | 50+ | GetHealth, SetPower, AddAura, GetDistanceTo |
| Spell | 18+ | GetCaster, Cancel, GetTarget, IsPositiveSpell |
| DB2 | 30+ | GetSpellName, GetItemName, GetMapName, HasDB2StoreEntry |
| Database | 10+ | Query, Execute, EscapeString (Character/World/Login DB) |
| Packet | 10+ | Read/Write uint/float/string/bits, SendPacketToPlayer |
| Spawn | 15+ | SpawnCreature, ConfigureCreature, MoveRandom, ScheduleEvent |

For the complete API reference, see [angelscripts/README.md](AngelScript/angelscripts/README.md).

## License

AngelCore is based on TrinityCore which is released under the GNU General Public License v2.
The AngelScript SDK is released under the zlib license.
All custom AngelScript code is provided as-is.
