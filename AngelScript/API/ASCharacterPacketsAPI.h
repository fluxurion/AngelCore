/*
 * AngelScript Character Packets API
 * Extracted from AngelScriptMgr.cpp monolith split
 */

#ifndef ASCHARACTERPACKETSAPI_H
#define ASCHARACTERPACKETSAPI_H

#ifdef ANGELSCRIPT_INTEGRATION

class asIScriptEngine;

namespace WorldPackets { namespace Character { class EnumCharactersResult; } }

namespace AngelScript
{
    void RegisterCharacterPacketsAPI();
    void CleanupWarbandGroupStorage(WorldPackets::Character::EnumCharactersResult* result);
}

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASCHARACTERPACKETSAPI_H
