# AngelScript Integration

This directory contains the C++ integration layer that embeds AngelScript into TrinityCore.

## Documentation

- **[angelscripts/README.md](angelscripts/README.md)** — Complete API reference, hook system, AngelDB, BattlePay, Warband, commands
- **[angelscripts/AngelDB/README.md](angelscripts/AngelDB/README.md)** — SQL update system documentation
- **[angelscripts/examples/README.md](angelscripts/examples/README.md)** — Example scripts by category
- **[../README.md](../README.md)** — Project overview

## Directory Structure

```
AngelScript/
├── API/                  — C++ binding code (Player, Creature, Spell, Packet, AngelDB, ...)
├── Hooks/                — Hook manager infrastructure
├── Dispatch/             — TC ScriptObject bridge classes
├── SDK/                  — AngelScript SDK (compiler & VM, v2.38.0)
├── AngelScriptMgr.cpp    — Engine lifecycle: init, load, reload, shutdown
├── CMakeLists.txt        — Build configuration
└── angelscripts/         — All .as scripts → see angelscripts/README.md
```
