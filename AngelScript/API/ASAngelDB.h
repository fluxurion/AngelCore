/*
 * ASAngelDB.h — Independent MySQL Database for AngelScript
 * Connection credentials are read from worldserver.conf (WorldDatabaseInfo).
 * Only the database name differs (default: "angelcore_scripts").
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

struct MYSQL;
struct MYSQL_RES;
class asIScriptEngine;

namespace AngelScript
{
    class ASAngelDB
    {
    public:
        static ASAngelDB& Instance();

        // Auto-initialize: reads credentials from worldserver.conf WorldDatabaseInfo,
        // replaces DB name with default ("angelcore_scripts"), creates DB if needed.
        // Called automatically during RegisterAngelDBAPI().
        // Returns true if connected.
        bool AutoInitialize();

        // Manual initialize: allows AS to override the database name or credentials.
        // If dbName is empty, uses default "angelcore_scripts".
        bool Initialize(std::string host, std::string port,
                        std::string user, std::string pass,
                        std::string dbName = "angelcore_scripts");

        void Shutdown();
        bool IsConnected() const;

        // Execute a raw SQL statement (INSERT/UPDATE/DELETE). Returns true on success.
        bool Execute(const std::string& sql);

        // Escape a string for safe SQL embedding.
        std::string EscapeString(const std::string& str);

        // Get last MySQL error message.
        std::string GetLastError();

        // Run all .sql files from baseDir/pending/, executing each against AngelDB.
        // Files that execute successfully are moved to baseDir/applied/.
        // Returns the number of files executed.
        uint32 RunPendingUpdates(const std::string& baseDir);

    private:
        ASAngelDB() = default;
        ~ASAngelDB();
        ASAngelDB(const ASAngelDB&) = delete;
        ASAngelDB& operator=(const ASAngelDB&) = delete;

        void ShutdownInternal();
        bool ExecuteLocked(const std::string& sql);
        MYSQL_RES* QueryLocked(const std::string& sql);
        bool ConnectWithRetry(const std::string& host, unsigned int port,
                              const std::string& user, const std::string& pass,
                              const std::string& dbName);

        MYSQL* _mysql = nullptr;
        std::mutex _mutex;
        std::atomic<bool> _connected{false};

        // Allow AS binding functions to access _mutex and QueryLocked
        friend struct ASAngelDBResult;
        friend ASAngelDBResult AS_AngelDB_Query(const std::string& sql);
    };

    void RegisterAngelDBAPI(asIScriptEngine* engine);

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASANGELDB_H
