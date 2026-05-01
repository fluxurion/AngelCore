#ifndef ASUNITAPI_H
#define ASUNITAPI_H

#ifdef ANGELSCRIPT_INTEGRATION

class asIScriptEngine;

namespace AngelScript
{
    void RegisterUnitAPI(asIScriptEngine* engine);
    void RegisterAuraAPI(asIScriptEngine* engine);
}

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASUNITAPI_H
