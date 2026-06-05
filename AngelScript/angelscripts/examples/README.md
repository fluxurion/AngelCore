# AngelScript Examples

These scripts are for learning — they are **not loaded** by the server (the `examples/` directory is excluded from compilation). Copy the ones you want to use into the parent `angelscripts/` directory.

## AngelDB Persistence (NEW)

All spawn functions now auto-persist to AngelDB by default. Spawns survive server restarts!
Use `persist = false` for temporary spawns:
```angelscript
SpawnCreature(entry, map, x, y, z, o, phaseId, respawn, persist = false)
```

## GUID Separation

AngelCore spawns use an isolated GUID range (bit 39 set) to avoid collision with TrinityCore:
- TC spawn GUIDs:  `0x0000000001` .. `0x0000007FFFFFFFFF`
- AS spawn GUIDs:  `0x0000008000000001` .. `0x000000FFFFFFFFFF`

Use `IsAngelCoreSpawn(spawnId)` to check if a spawn belongs to AngelCore.

## Directory Structure

| Folder | Topic |
|--------|-------|
| `01_basics/` | Hello World, Print, global functions, DB queries, DB2 lookups |
| `02_spawning/` | SpawnCreature, SpawnCreatureEx, phased spawns, gameobjects, bulk arrays |
| `03_hooks/` | Player hooks (login/logout/chat/duel), creature hooks, spell hooks |
| `04_timed_events/` | ScheduleEvent, RepeatEvent, patrol NPCs, periodic emotes |
| `05_encounters/` | Quest boss with patrol+combat, multi-phase boss with HP% transitions |
| `06_packets/` | Packet receive/send intercept, opcode logging, packet blocking |

## Progression

Start here and work your way up:

1. **`01_basics/hello_world.as`** — Simplest script, just prints to console
2. **`02_spawning/bare_minimum.as`** — Spawn a creature with 1 line
3. **`02_spawning/full_config.as`** — SpawnCreatureEx with level/faction/flags/gossip
4. **`03_hooks/player_login.as`** — Welcome message, broadcast on login
5. **`04_timed_events/simple_timer.as`** — NPC that waves every 10-30 seconds
6. **`04_timed_events/patrol_npc.as`** — NPC walking a patrol route
7. **`05_encounters/quest_boss.as`** — Full boss: patrol → combat → spells → death → respawn
8. **`05_encounters/multi_phase_boss.as`** — 3-phase boss with HP% transitions

## Quick Reference

### Spawning
```angelscript
SpawnCreature(entry, map, x, y, z, o, phaseId, respawn, persist=true)                   // bare
SpawnCreatureEx(entry, map, x, y, z, o, phase, respawn, lvl, fac, flags, goss, eq, react, persist=true) // full
SpawnGameObject(entry, map, x, y, z, o, phaseId, respawn, state, persist=true)
ConfigureCreature(creature, lvl, fac, flags, goss, eq, react)                          // batch-config

// Utility
IsAngelCoreSpawn(spawnId)            // check if spawn is AngelCore-managed
FindSpawnedCreature(spawnId, mapId)  // find by spawnId (not TC guid)
FindSpawnedGameObject(spawnId, mapId)
RemoveSpawnedCreature(creature)      // permanent removal (deletes from AngelDB too)
```

### Timers
```angelscript
creature.ScheduleEvent(eventId, delayMs)
creature.ScheduleEventRandom(eventId, minMs, maxMs)
creature.RepeatEvent(delayMs)
creature.CancelEvent(eventId)
creature.CancelEventGroup(groupId)
creature.HasTimers()
creature.UpdateTimers(diffMs)     // returns next event ID or 0
```

### Scripting
```angelscript
creature.Say("text")
creature.Yell("text")
creature.DoEmote(emoteId)
creature.CastSpellSelf(spellId)
creature.MovePoint(id, x, y, z)
creature.MoveRandom(distance)
creature.SetLevel(level)
creature.SetFaction(factionTemplateId)
```
