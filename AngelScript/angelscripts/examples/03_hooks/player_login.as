/*
 * Example 03: Player Login / Logout Hooks
 * React when players join or leave the server.
 */

#include "../includes/ScriptFramework.as"

void OnLogin(Player@ player)
{
    if (player is null) return;

    // Send a welcome message to the player
    string msg = "|cff00FF00Welcome back, " + player.GetName() + "!|r";
    SendSystemMessage(player, msg);

    // Broadcast to the world
    string announce = "|cffFFD800" + player.GetName() + " has entered the world.|r";
    SendWorldText(announce);

    Print("[Login] " + player.GetName() +
          " | Level " + player.GetLevel() +
          " | Class " + GetClassName(player.GetClass()) +
          " | Account " + player.GetAccountId());
}

void OnLogout(Player@ player)
{
    if (player is null) return;
    Print("[Logout] " + player.GetName());
}

void OnLevelUp(Player@ player, uint8 oldLevel)
{
    if (player is null) return;
    Print("[LevelUp] " + player.GetName() + " reached level " + player.GetLevel());

    // Send a congratulation
    SendSystemMessage(player, "|cffFFD700Congratulations on reaching level " +
                      player.GetLevel() + "!|r");
}

void OnDeath(Player@ player, Unit@ killer)
{
    if (player is null) return;

    string killerName = "unknown";
    if (killer !is null)
        killerName = killer.GetName();

    Print("[Death] " + player.GetName() + " was killed by " + killerName);
}

void main()
{
    RegisterPlayerScript(PLAYER_ON_LOGIN,    @OnLogin);
    RegisterPlayerScript(PLAYER_ON_LOGOUT,   @OnLogout);
    RegisterPlayerScript(PLAYER_ON_LEVEL_UP, @OnLevelUp);
    RegisterPlayerScript(PLAYER_ON_DEATH,    @OnDeath);
    Print("[PlayerHooks] Registered: login, logout, level up, death");
}
