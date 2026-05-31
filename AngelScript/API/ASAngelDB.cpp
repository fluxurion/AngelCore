/*
 * ASAngelDB.cpp — Independent MySQL Database for AngelScript
 *
 * Connection credentials come from worldserver.conf (WorldDatabaseInfo):
 *   WorldDatabaseInfo = "host;port;user;password;world_db"
 * AngelDB reuses host/port/user/password and replaces the database name
 * with "angelcore_db" (or a custom name set via AS).
 *
 * If the database doesn't exist, it is auto-created on first connect.
 *
 * Zero dependency on TrinityCore DatabaseWorkerPool / MySQLConnection.
 * Uses raw libmysqlclient (blocking) with std::mutex guarding.
 */

#ifndef ANGELSCRIPT_INTEGRATION
    #error "ANGELSCRIPT_INTEGRATION macro must be defined"
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#include "ASAngelDB.h"
#include "Log.h"
#include "Config.h"   // sConfigMgr for reading worldserver.conf
#include "AngelScriptMgr.h" // GetScriptPath() for updates dir

// MySQL C API — include path provided by 'mysql' target in CMakeLists.txt
#include <mysql.h>

// AngelScript SDK
#pragma push_macro("IN")
#pragma push_macro("OUT")
#pragma push_macro("OPTIONAL")
#undef IN
#undef OUT
#undef OPTIONAL
#include <angelscript.h>
#pragma pop_macro("OPTIONAL")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

namespace AngelScript
{
    // ========================================================================
    // ASAngelDBResult — Thin wrapper around MYSQL_RES for AngelScript
    // ========================================================================
    struct ASAngelDBResult
    {
        MYSQL_RES*      res = nullptr;
        MYSQL_ROW       currentRow = nullptr;
        unsigned long*  currentLengths = nullptr;
        uint32          fieldCount = 0;
        uint64          rowCount = 0;
        MYSQL_FIELD*    fields = nullptr;
        bool            ownsResult = true;

        ~ASAngelDBResult()
        {
            if (ownsResult && res)
                mysql_free_result(res);
            res = nullptr;
        }

        ASAngelDBResult() = default;
        ASAngelDBResult(const ASAngelDBResult&) = delete;
        ASAngelDBResult& operator=(const ASAngelDBResult&) = delete;
        ASAngelDBResult(ASAngelDBResult&& other) noexcept
            : res(other.res), currentRow(other.currentRow), currentLengths(other.currentLengths)
            , fieldCount(other.fieldCount), rowCount(other.rowCount)
            , fields(other.fields), ownsResult(other.ownsResult)
        {
            other.res = nullptr;
            other.ownsResult = false;
        }
    };

    // ========================================================================
    // Helpers: parse "host;port;user;pass;db" format from worldserver.conf
    // ========================================================================
    static std::vector<std::string> SplitInfoString(const std::string& str)
    {
        std::vector<std::string> parts;
        std::istringstream ss(str);
        std::string part;
        while (std::getline(ss, part, ';'))
            parts.push_back(part);
        return parts;
    }

    // ========================================================================
    // Singleton
    // ========================================================================
    ASAngelDB& ASAngelDB::Instance()
    {
        static ASAngelDB instance;
        return instance;
    }

    ASAngelDB::~ASAngelDB()
    {
        Shutdown();
    }

    // ========================================================================
    // AutoInitialize — reads worldserver.conf, connects, auto-creates DB
    // ========================================================================
    bool ASAngelDB::AutoInitialize()
    {
        std::string infoStr = sConfigMgr->GetStringDefault("WorldDatabaseInfo", "");
        if (infoStr.empty())
        {
            TC_LOG_WARN("server.angelscript",
                "[AngelDB] WorldDatabaseInfo not found in worldserver.conf — cannot auto-initialize");
            return false;
        }

        auto parts = SplitInfoString(infoStr);
        if (parts.size() < 5)
        {
            TC_LOG_ERROR("server.angelscript",
                "[AngelDB] WorldDatabaseInfo has unexpected format: '{}' (expected host;port;user;pass;db)", infoStr);
            return false;
        }

        std::string host = parts[0];
        std::string port = parts[1];
        std::string user = parts[2];
        std::string pass = parts[3];
        // parts[4] is the world DB name — we replace it

        return Initialize(host, port, user, pass, "angelcore_db");
    }

    // ========================================================================
    // Initialize — connect with auto-create if DB doesn't exist
    // ========================================================================
    bool ASAngelDB::Initialize(std::string host, std::string port,
                                std::string user, std::string pass,
                                std::string dbName)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if ((MYSQL*)_mysql)
        {
            TC_LOG_WARN("server.angelscript", "[AngelDB] Already initialized, reconnecting...");
            ShutdownInternal();
        }

        if (dbName.empty())
            dbName = "angelcore_db";

        unsigned int portNum = static_cast<unsigned int>(
            std::stoul(port.empty() ? "3306" : port));

        TC_LOG_INFO("server.angelscript",
            "[AngelDB] Connecting to {}@{}:{}/{} ...", user, host, portNum, dbName);

        // Try connecting directly to the target database first
        if (ConnectWithRetry(host, portNum, user, pass, dbName))
            return true;

        // If the database doesn't exist, create it and reconnect
        unsigned int lastErr = mysql_errno((MYSQL*)_mysql);
        if (lastErr == 1049)  // ER_BAD_DB_ERROR: Unknown database
        {
            TC_LOG_INFO("server.angelscript",
                "[AngelDB] Database '{}' does not exist — creating...", dbName);

            // Close failed connection
            mysql_close((MYSQL*)_mysql);
            _mysql = nullptr;

            // Connect without a specific database (use 'mysql' system DB)
            _mysql = mysql_init(nullptr);
            if (!(MYSQL*)_mysql) return false;

            unsigned int timeout = 10;
            mysql_options((MYSQL*)_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
            mysql_options((MYSQL*)_mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

            MYSQL* conn = mysql_real_connect(
                (MYSQL*)_mysql, host.c_str(), user.c_str(), pass.c_str(),
                nullptr,  // no database
                portNum, nullptr, 0);

            if (!conn)
            {
                TC_LOG_ERROR("server.angelscript",
                    "[AngelDB] Cannot connect to MySQL server (no DB): {}",
                    mysql_error((MYSQL*)_mysql));
                mysql_close((MYSQL*)_mysql);
                _mysql = nullptr;
                return false;
            }

            // Create the database
            std::string createSQL =
                "CREATE DATABASE IF NOT EXISTS `" + dbName +
                "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";

            if (mysql_query((MYSQL*)_mysql, createSQL.c_str()) != 0)
            {
                TC_LOG_ERROR("server.angelscript",
                    "[AngelDB] CREATE DATABASE failed: {}", mysql_error((MYSQL*)_mysql));
                mysql_close((MYSQL*)_mysql);
                _mysql = nullptr;
                return false;
            }

            TC_LOG_INFO("server.angelscript",
                "[AngelDB] Database '{}' created successfully", dbName);

            // Close the no-DB connection
            mysql_close((MYSQL*)_mysql);
            _mysql = nullptr;

            // Reconnect with the new database
            return ConnectWithRetry(host, portNum, user, pass, dbName);
        }

        // Some other error
        TC_LOG_ERROR("server.angelscript",
            "[AngelDB] Connection failed: {}@{}:{}/{} — {}",
            user, host, portNum, dbName,
            (MYSQL*)_mysql ? mysql_error((MYSQL*)_mysql) : "unknown");
        if ((MYSQL*)_mysql) { mysql_close((MYSQL*)_mysql); _mysql = nullptr; }
        return false;
    }

    bool ASAngelDB::ConnectWithRetry(const std::string& host, unsigned int port,
                                      const std::string& user, const std::string& pass,
                                      const std::string& dbName)
    {
        _mysql = mysql_init(nullptr);
        if (!(MYSQL*)_mysql) return false;

        unsigned int timeout = 10;
        mysql_options((MYSQL*)_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        bool reconnect = true;
        mysql_options((MYSQL*)_mysql, MYSQL_OPT_RECONNECT, &reconnect);
        mysql_options((MYSQL*)_mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

        MYSQL* conn = mysql_real_connect(
            (MYSQL*)_mysql, host.c_str(), user.c_str(), pass.c_str(),
            dbName.c_str(), port, nullptr, CLIENT_MULTI_STATEMENTS);

        if (!conn)
        {
            // Don't close _mysql here — caller may check mysql_errno
            return false;
        }

        _connected = true;
        TC_LOG_INFO("server.angelscript",
            "[AngelDB] Connected to {}@{}:{}/{} (server version: {})",
            user, host, port, dbName, mysql_get_server_info((MYSQL*)_mysql));
        return true;
    }

    void ASAngelDB::ShutdownInternal()
    {
        if ((MYSQL*)_mysql)
        {
            mysql_close((MYSQL*)_mysql);
            _mysql = nullptr;
        }
        _connected = false;
    }

    void ASAngelDB::Shutdown()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        ShutdownInternal();
        TC_LOG_INFO("server.angelscript", "[AngelDB] Connection closed");
    }

    bool ASAngelDB::IsConnected() const
    {
        return _connected.load();
    }

    // ========================================================================
    // Locked internal helpers
    // ========================================================================
    bool ASAngelDB::ExecuteLocked(const std::string& sql)
    {
        if (!(MYSQL*)_mysql || !_connected) return false;
        if (mysql_query((MYSQL*)_mysql, sql.c_str()) != 0)
        {
            TC_LOG_ERROR("server.angelscript", "[AngelDB] Execute() failed: {} — {}",
                mysql_error((MYSQL*)_mysql), sql);
            return false;
        }
        return true;
    }

    void* ASAngelDB::QueryLocked(const std::string& sql)
    {
        MYSQL* m = static_cast<MYSQL*>(_mysql);
        if (!m || !_connected) return nullptr;
        if (mysql_query(m, sql.c_str()) != 0)
        {
            TC_LOG_ERROR("server.angelscript", "[AngelDB] Query() failed: {} — {}",
                mysql_error(m), sql);
            return nullptr;
        }
        MYSQL_RES* result = mysql_store_result(m);
        if (!result && mysql_field_count(m) > 0)
            TC_LOG_ERROR("server.angelscript", "[AngelDB] mysql_store_result() failed: {}",
                mysql_error(m));
        return result;
    }

    // ========================================================================
    // Public Execute / Utils
    // ========================================================================
    bool ASAngelDB::Execute(const std::string& sql)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return ExecuteLocked(sql);
    }

    std::string ASAngelDB::EscapeString(const std::string& str)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!(MYSQL*)_mysql) return str;
        std::vector<char> buf(str.size() * 2 + 1);
        size_t len = mysql_real_escape_string((MYSQL*)_mysql, buf.data(), str.c_str(), str.size());
        return std::string(buf.data(), len);
    }

    std::string ASAngelDB::GetLastError()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!(MYSQL*)_mysql) return "Not connected";
        return std::string(mysql_error((MYSQL*)_mysql));
    }

    // ========================================================================
    // RunPendingUpdates - SQL migration system
    // Scans baseDir/pending/ for .sql files, executes each, moves to applied/.
    // ========================================================================
    static void MoveAppliedFile(const fs::path& src, const fs::path& appliedDir, const std::string& filename)
    {
        fs::path dest = appliedDir / filename;
        std::error_code ec;
        fs::rename(src, dest, ec);
    }

    uint32 ASAngelDB::RunPendingUpdates(const std::string& baseDir)
    {
        fs::path pendingDir = fs::path(baseDir) / "pending";
        fs::path appliedDir = fs::path(baseDir) / "applied";

        if (!fs::exists(pendingDir) || !fs::is_directory(pendingDir))
        {
            TC_LOG_INFO("server.angelscript",
                "[AngelDB] Pending updates dir not found: {} — copy AngelDB/ to your binary output directory", pendingDir.string());
            return 0;
        }

        // Collect .sql files, sorted by name
        std::vector<fs::path> sqlFiles;
        for (auto const& entry : fs::directory_iterator(pendingDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".sql")
                sqlFiles.push_back(entry.path());
        }
        std::sort(sqlFiles.begin(), sqlFiles.end());

        if (sqlFiles.empty())
        {
            TC_LOG_INFO("server.angelscript", "[AngelDB] No pending .sql files in {}", pendingDir.string());
            return 0;
        }

        // Ensure applied dir exists
        if (!fs::exists(appliedDir))
            fs::create_directories(appliedDir);

        uint32 executed = 0;

        for (auto const& filePath : sqlFiles)
        {
            std::string filename = filePath.filename().string();
            TC_LOG_INFO("server.angelscript", "[AngelDB] Executing update: {}", filename);

            // Read file content
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                TC_LOG_ERROR("server.angelscript", "[AngelDB] Cannot open file: {}", filePath.string());
                continue;
            }
            std::string content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            file.close();

            if (content.empty())
            {
                TC_LOG_WARN("server.angelscript", "[AngelDB] Empty file: {}", filename);
                // Move empty files anyway (they were "applied")
                MoveAppliedFile(filePath, appliedDir, filename);
                executed++;
                continue;
            }

            // Execute entire file as multi-statement query (CLIENT_MULTI_STATEMENTS enabled)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!(MYSQL*)_mysql || !_connected)
                {
                    TC_LOG_ERROR("server.angelscript", "[AngelDB] Not connected, skipping: {}", filename);
                    break;
                }

                MYSQL* m = static_cast<MYSQL*>(_mysql);
                if (mysql_real_query(m, content.c_str(), content.size()) != 0)
                {
                    TC_LOG_ERROR("server.angelscript",
                        "[AngelDB] SQL error in {}: {}", filename, mysql_error(m));
                    TC_LOG_ERROR("server.angelscript",
                        "[AngelDB] Update {} FAILED — left in pending/", filename);
                    continue;
                }

                // Consume all result sets from multi-statement execution
                do {
                    MYSQL_RES* res = mysql_store_result(m);
                    if (res) mysql_free_result(res);
                } while (mysql_next_result(m) == 0);
            }

            // Move from pending/ to applied/
            fs::path dest = appliedDir / filename;

            // If a file with same name already exists in applied, rename with timestamp
            if (fs::exists(dest))
            {
                std::string base = filePath.stem().string();
                dest = appliedDir / (base + "_" +
                    std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".sql");
            }

            std::error_code ec;
            fs::rename(filePath, dest, ec);
            if (ec)
            {
                TC_LOG_ERROR("server.angelscript",
                    "[AngelDB] Cannot move {} to applied/: {}", filename, ec.message());
            }
            else
            {
                TC_LOG_INFO("server.angelscript", "[AngelDB] Update {} applied successfully", filename);
                executed++;
            }
        }

        TC_LOG_INFO("server.angelscript", "[AngelDB] {} update(s) executed", executed);
        return executed;
    }

    // ========================================================================
    // AngelScript Binding Functions
    // ========================================================================

    static bool AS_AngelDB_AutoInitialize()
    {
        return ASAngelDB::Instance().AutoInitialize();
    }

    static bool AS_AngelDB_Initialize(const std::string& host, const std::string& port,
                                       const std::string& user, const std::string& pass,
                                       const std::string& db)
    {
        return ASAngelDB::Instance().Initialize(host, port, user, pass, db);
    }

    static bool AS_AngelDB_IsConnected() { return ASAngelDB::Instance().IsConnected(); }
    static void AS_AngelDB_Shutdown()    { ASAngelDB::Instance().Shutdown(); }
    static uint32 AS_AngelDB_RunPendingUpdates(const std::string& baseDir)
    {
        return ASAngelDB::Instance().RunPendingUpdates(baseDir);
    }

    static bool AS_AngelDB_Execute(const std::string& sql)
    {
        return ASAngelDB::Instance().Execute(sql);
    }

    ASAngelDBResult AS_AngelDB_Query(const std::string& sql)
    {
        ASAngelDBResult result;
        {
            std::lock_guard<std::mutex> lock(ASAngelDB::Instance().GetMutex());
            result.res = static_cast<MYSQL_RES*>(ASAngelDB::Instance().QueryLocked(sql));
        }
        if (result.res)
        {
            result.fieldCount = mysql_num_fields(result.res);
            result.rowCount   = mysql_num_rows(result.res);
            result.fields     = mysql_fetch_fields(result.res);
        }
        return result;
    }

    static std::string AS_AngelDB_EscapeString(const std::string& str)
    {
        return ASAngelDB::Instance().EscapeString(str);
    }

    static std::string AS_AngelDB_GetLastError()
    {
        return ASAngelDB::Instance().GetLastError();
    }

    // ========================================================================
    // ASAngelDBResult methods
    // ========================================================================

    static uint64  R_GetRowCount(ASAngelDBResult& r)   { return r.rowCount; }
    static uint32  R_GetFieldCount(ASAngelDBResult& r) { return r.fieldCount; }

    static bool R_NextRow(ASAngelDBResult& r)
    {
        if (!r.res) return false;
        r.currentRow = mysql_fetch_row(r.res);
        if (r.currentRow)
            r.currentLengths = mysql_fetch_lengths(r.res);
        return r.currentRow != nullptr;
    }

    static bool R_IsNull(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount) return true;
        return r.currentRow[idx] == nullptr;
    }

    static std::string R_GetString(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount || !r.currentRow[idx])
            return "";
        return std::string(r.currentRow[idx],
            r.currentLengths ? r.currentLengths[idx] : 0);
    }

    static int32  R_GetInt32(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount || !r.currentRow[idx]) return 0;
        return static_cast<int32>(std::strtol(r.currentRow[idx], nullptr, 10));
    }
    static uint32 R_GetUInt32(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount || !r.currentRow[idx]) return 0;
        return static_cast<uint32>(std::strtoul(r.currentRow[idx], nullptr, 10));
    }
    static int64  R_GetInt64(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount || !r.currentRow[idx]) return 0;
        return static_cast<int64>(std::strtoll(r.currentRow[idx], nullptr, 10));
    }
    static uint64 R_GetUInt64(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount || !r.currentRow[idx]) return 0;
        return static_cast<uint64>(std::strtoull(r.currentRow[idx], nullptr, 10));
    }
    static float  R_GetFloat(ASAngelDBResult& r, uint32 idx)
    {
        if (!r.currentRow || idx >= r.fieldCount || !r.currentRow[idx]) return 0.0f;
        return std::strtof(r.currentRow[idx], nullptr);
    }
    static std::string R_GetFieldName(ASAngelDBResult& r, uint32 idx)
    {
        if (idx >= r.fieldCount || !r.fields) return "";
        return r.fields[idx].name ? r.fields[idx].name : "";
    }

    static void R_DefaultCtor(ASAngelDBResult* mem) { new(mem) ASAngelDBResult(); }
    static void R_CopyCtor(ASAngelDBResult* mem, ASAngelDBResult&) { new(mem) ASAngelDBResult(); }
    static void R_Dtor(ASAngelDBResult* mem) { mem->~ASAngelDBResult(); }

    // opAssign — frees old MYSQL_RES, takes ownership from source (move semantics)
    static ASAngelDBResult& R_OpAssign(ASAngelDBResult& self, ASAngelDBResult& other)
    {
        if (&self != &other)
        {
            if (self.ownsResult && self.res)
                mysql_free_result(self.res);
            self.res = other.res;
            self.currentRow = other.currentRow;
            self.currentLengths = other.currentLengths;
            self.fieldCount = other.fieldCount;
            self.rowCount = other.rowCount;
            self.fields = other.fields;
            self.ownsResult = other.ownsResult;
            other.res = nullptr;
            other.ownsResult = false;
            other.currentRow = nullptr;
            other.currentLengths = nullptr;
        }
        return self;
    }

    // ========================================================================
    // Register API
    // ========================================================================
    void RegisterAngelDBAPI(asIScriptEngine* engine)
    {
        int r;

        // ---- ASAngelDBResult value type ----
        r = engine->RegisterObjectType("AngelDBResult", sizeof(ASAngelDBResult),
            asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
        r = engine->RegisterObjectBehaviour("AngelDBResult",
            asBEHAVE_CONSTRUCT,  "void f()",
            asFUNCTION(R_DefaultCtor), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectBehaviour("AngelDBResult",
            asBEHAVE_CONSTRUCT,  "void f(const AngelDBResult& in)",
            asFUNCTION(R_CopyCtor),   asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectBehaviour("AngelDBResult",
            asBEHAVE_DESTRUCT,   "void f()",
            asFUNCTION(R_Dtor),        asCALL_CDECL_OBJFIRST);
        // opAssign — enables result = AngelDB_Query(...) reassignment
        r = engine->RegisterObjectMethod("AngelDBResult",
            "AngelDBResult& opAssign(const AngelDBResult& in)",
            asFUNCTION(R_OpAssign), asCALL_CDECL_OBJFIRST);

        r = engine->RegisterObjectMethod("AngelDBResult",
            "uint64 GetRowCount() const",      asFUNCTION(R_GetRowCount), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "uint32 GetFieldCount() const",    asFUNCTION(R_GetFieldCount), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "bool NextRow()",                  asFUNCTION(R_NextRow), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "bool IsNull(uint32) const",       asFUNCTION(R_IsNull), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "string GetString(uint32) const",  asFUNCTION(R_GetString), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "int32 GetInt32(uint32) const",    asFUNCTION(R_GetInt32), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "uint32 GetUInt32(uint32) const",  asFUNCTION(R_GetUInt32), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "int64 GetInt64(uint32) const",    asFUNCTION(R_GetInt64), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "uint64 GetUInt64(uint32) const",  asFUNCTION(R_GetUInt64), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "float GetFloat(uint32) const",    asFUNCTION(R_GetFloat), asCALL_CDECL_OBJFIRST);
        r = engine->RegisterObjectMethod("AngelDBResult",
            "string GetFieldName(uint32) const", asFUNCTION(R_GetFieldName), asCALL_CDECL_OBJFIRST);

        // ---- Global AngelDB functions ----
        r = engine->RegisterGlobalFunction(
            "bool AngelDB_AutoInitialize()",
            asFUNCTION(AS_AngelDB_AutoInitialize), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "bool AngelDB_Initialize(const string& in,const string& in,const string& in,const string& in,const string& in)",
            asFUNCTION(AS_AngelDB_Initialize), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "bool AngelDB_IsConnected()",
            asFUNCTION(AS_AngelDB_IsConnected), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "void AngelDB_Shutdown()",
            asFUNCTION(AS_AngelDB_Shutdown), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "AngelDBResult AngelDB_Query(const string& in)",
            asFUNCTION(AS_AngelDB_Query), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "bool AngelDB_Execute(const string& in)",
            asFUNCTION(AS_AngelDB_Execute), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "string AngelDB_EscapeString(const string& in)",
            asFUNCTION(AS_AngelDB_EscapeString), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "string AngelDB_GetLastError()",
            asFUNCTION(AS_AngelDB_GetLastError), asCALL_CDECL);
        r = engine->RegisterGlobalFunction(
            "uint32 AngelDB_RunPendingUpdates(const string& in)",
            asFUNCTION(AS_AngelDB_RunPendingUpdates), asCALL_CDECL);

        // ---- Auto-initialize on registration ----
        ASAngelDB::Instance().AutoInitialize();

        // ---- Auto-run pending SQL updates ----
        // Resolves <binary_dir>/angelscripts/AngelDB/ on both Linux and Windows.
        // Override via worldserver.conf: AngelScript.UpdatesDir = "/absolute/path"
        std::string updatesDir = sConfigMgr->GetStringDefault("AngelScript.UpdatesDir", "");
        if (updatesDir.empty())
        {
            fs::path scriptDir = fs::path(AngelScriptMgr::instance()->GetScriptPath());
            if (!scriptDir.is_absolute())
            {
#ifdef _WIN32
                char exeBuf[MAX_PATH];
                GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
                scriptDir = fs::path(exeBuf).parent_path() / scriptDir;
#else
                scriptDir = fs::read_symlink("/proc/self/exe").parent_path() / scriptDir;
#endif
            }
            updatesDir = (scriptDir / "AngelDB").string();
        }
        ASAngelDB::Instance().RunPendingUpdates(updatesDir);

        TC_LOG_INFO("server.angelscript", "AngelDB API registered (auto-initialized from worldserver.conf)");
    }

} // namespace AngelScript
