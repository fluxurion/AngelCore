/*
 * AngelDB Example - Independent MySQL Database Usage
 *
 * The AngelDB connection is auto-initialized from worldserver.conf
 * during server startup. It reuses the same MySQL server credentials
 * as the world/character/login databases.
 *
 * The database "angelcore_db" is auto-created if it doesn't exist.
 *
 * TABLE SETUP (runs automatically on startup):
 *
 *   CREATE TABLE IF NOT EXISTS as_custom_spawns (
 *       id       INT AUTO_INCREMENT PRIMARY KEY,
 *       entry    INT UNSIGNED NOT NULL,
 *       map      INT UNSIGNED NOT NULL DEFAULT 0,
 *       pos_x    FLOAT NOT NULL DEFAULT 0,
 *       pos_y    FLOAT NOT NULL DEFAULT 0,
 *       pos_z    FLOAT NOT NULL DEFAULT 0,
 *       orient   FLOAT NOT NULL DEFAULT 0,
 *       name     VARCHAR(255) NOT NULL DEFAULT ''
 *   ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
 */

#include "../includes/ScriptFramework.as"
#include "../Config.as"

// ============================================================================
// INITIALIZATION - Called when this script module loads
// The C++ layer already called AngelDB_AutoInitialize() during API registration,
// so the connection should already be live.
// ============================================================================
void main()
{
    Print("[AngelDB-Example] Checking AngelDB connection...");

    if (AngelDB_IsConnected())
    {
        Print("[AngelDB-Example] AngelDB is connected (auto-initialized from worldserver.conf)");
        EnsureTables();
        RunTests();
    }
    else
    {
        Print("[AngelDB-Example] AngelDB is NOT connected! Check server logs.");
    }
}

// ============================================================================
// TABLE SETUP - Create tables if they don't exist
// ============================================================================
void EnsureTables()
{
    AngelDB_Execute(
        "CREATE TABLE IF NOT EXISTS as_custom_spawns ("
        "  id       INT AUTO_INCREMENT PRIMARY KEY,"
        "  entry    INT UNSIGNED NOT NULL,"
        "  map      INT UNSIGNED NOT NULL DEFAULT 0,"
        "  pos_x    FLOAT NOT NULL DEFAULT 0,"
        "  pos_y    FLOAT NOT NULL DEFAULT 0,"
        "  pos_z    FLOAT NOT NULL DEFAULT 0,"
        "  orient   FLOAT NOT NULL DEFAULT 0,"
        "  name     VARCHAR(255) NOT NULL DEFAULT ''"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
    );
    Print("[AngelDB-Example] Tables ensured");
}

// ============================================================================
// TEST - INSERT, SELECT, UPDATE
// ============================================================================
void RunTests()
{
    // ---- INSERT with string escaping ----
    string safeName = AngelDB_EscapeString("TestNPC_Sindragosa");
    if (AngelDB_Execute(
        "INSERT INTO as_custom_spawns (entry, map, pos_x, pos_y, pos_z, orient, name) "
        "VALUES (36853, 571, 5800.0, -1200.0, 450.0, 3.14, '" + safeName + "') "
        "ON DUPLICATE KEY UPDATE name=name"))
    {
        Print("[AngelDB-Example] INSERT OK");
    }
    else
    {
        Print("[AngelDB-Example] INSERT FAILED: " + AngelDB_GetLastError());
    }

    // ---- SELECT with row iteration ----
    AngelDBResult r = AngelDB_Query(
        "SELECT id, entry, name, pos_x, pos_y, map FROM as_custom_spawns LIMIT 10");

    uint64 rows = r.GetRowCount();
    Print("[AngelDB-Example] SELECT returned " + rows + " row(s), " + r.GetFieldCount() + " fields");

    uint32 n = 0;
    while (r.NextRow() && n < 10)
    {
        Print("  [" + n + "] id=" + r.GetUInt32(0) +
              " entry=" + r.GetUInt32(1) +
              " name='" + r.GetString(2) + "'" +
              " pos=(" + r.GetFloat(3) + "," + r.GetFloat(4) + ")" +
              " map=" + r.GetUInt32(5));
        n++;
    }

    // ---- UPDATE ----
    AngelDB_Execute("UPDATE as_custom_spawns SET pos_x = 6000.0 WHERE entry = 36853");
    Print("[AngelDB-Example] Tests complete");
}

// ============================================================================
// REAL USE CASE - Load spawn data from AngelDB
// ============================================================================
void LoadSpawnsFromDatabase(uint32 mapId)
{
    if (!AngelDB_IsConnected()) return;

    string sql = "SELECT entry, pos_x, pos_y, pos_z, orient, name, map"
                 " FROM as_custom_spawns WHERE map = " + mapId;
    AngelDBResult r = AngelDB_Query(sql);

    uint32 spawned = 0;
    while (r.NextRow())
    {
        uint32 entry  = r.GetUInt32(0);
        float  posX   = r.GetFloat(1);
        float  posY   = r.GetFloat(2);
        float  posZ   = r.GetFloat(3);
        float  orient = r.GetFloat(4);
        string name   = r.GetString(5);

        Print("[AngelDB-Example] Would spawn " + name + " (entry=" + entry + ") on map " + mapId);
        // SpawnCreature(entry, mapId, posX, posY, posZ, orient, 0);
        spawned++;
    }
    Print("[AngelDB-Example] Loaded " + spawned + " spawn(s) for map " + mapId);
}
