/*
 * Example 01: Database Query
 * Run SQL queries from scripts.
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    // Execute an INSERT/UPDATE/DELETE
    CharacterExecute("INSERT INTO custom_startup_log (timestamp, msg) VALUES (UNIX_TIMESTAMP(), 'Server started from AngelScript')");

    // Query and read results
    QueryResult@ result = CharacterQuery("SELECT account, name FROM characters WHERE online = 1 LIMIT 5");
    if (result !is null)
    {
        uint32 count = 0;
        do
        {
            uint32 acc = result.GetUInt32(0);
            string name = result.GetString(1);
            Print("Online: " + name + " (account " + acc + ")");
            count++;
        }
        while (result.NextRow());
        Print("Total online: " + count);
    }
    else
    {
        Print("No online players or query failed");
    }
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
