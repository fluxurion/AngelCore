/*
 * Example 01: Global Functions
 * Shows Print, SendWorldText, GetGameTime, FindPlayerByName
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    Print("=== Server started! ===");

    // Send a message to all players online
    SendWorldText("|cff00FF00Server is now live!|r");

    // Server time
    uint32 time = GetGameTime();
    Print("Game time: " + time);

    // Player count
    Print("Players online: " + World_GetPlayerCount());
}

void OnShutdown()
{
    SendWorldText("|cffFF0000Server shutting down...|r");
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
    RegisterWorldScript(WORLD_ON_SHUTDOWN, @OnShutdown);
}
