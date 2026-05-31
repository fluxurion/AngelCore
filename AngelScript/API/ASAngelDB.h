/*
 * ASAngelDB.h — Independent MySQL Database for AngelScript
 * Connection credentials are read from worldserver.conf (WorldDatabaseInfo).
 * Only the database name differs (default: "angelcore_db").
 * The database is auto-created on first connect if it doesn't exist.
 *
 * Zero modification to TrinityCore factory files.
 */
#ifndef ASANGELDB_H
#define ASANGELDB_H

#ifdef ANGELSCRIPT_INTEGRATION

#include <string>
#include <mutex>
#include <atomic>
#include "Define.h"

class asIScriptEngine;
struct ASAngelDBResult;

namespace AngelScript
{
    class ASAngelDB
    {
    public:
        static ASAngelDB& Instance();

        bool AutoInitialize();
        bool Initialize(std::string host, std::string port,
                        std::string user, std::string pass,
                        std::string dbName = "angelcore_db");
        void Shutdown();
        bool IsConnected() const;
        bool Execute(const std::string& sql);
        std::string EscapeString(const std::string& str);
        std::string GetLastError();
        uint32 RunPendingUpdates(const std::string& baseDir);

        // Internal accessors for AS binding functions
        std::mutex& GetMutex() { return _mutex; }
        void* QueryLocked(const std::string& sql);

    private:
        ASAngelDB() = default;
        ~ASAngelDB();
        ASAngelDB(const ASAngelDB&) = delete;
        ASAngelDB& operator=(const ASAngelDB&) = delete;

        void ShutdownInternal();
        bool ExecuteLocked(const std::string& sql);
        bool ConnectWithRetry(const std::string& host, unsigned int port,
                              const std::string& user, const std::string& pass,
                              const std::string& dbName);

        void* _mysql = nullptr;
        std::mutex _mutex;
        std::atomic<bool> _connected{false};
    };

    void RegisterAngelDBAPI(asIScriptEngine* engine);

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASANGELDB_H
