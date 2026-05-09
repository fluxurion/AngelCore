/*
 * Example 03: Player Chat, Duel, Money Hooks
 */

#include "../includes/ScriptFramework.as"

void OnChat(Player@ player, uint32 type, uint32 lang, string& msg)
{
    if (player is null) return;

    // Log all chat
    Print("[Chat] " + player.GetName() + ": " + msg);

    // Block certain words (example)
    if (msg.find("badword") != -1)
        msg = "***filtered***";
}

void OnDuelEnd(Player@ winner, Player@ loser, uint32 type)
{
    if (winner is null || loser is null) return;

    string announce = winner.GetName() + " defeated " + loser.GetName() + " in a duel!";
    SendWorldText(announce);
}

void OnMoneyChange(Player@ player, int64 amount)
{
    if (player is null) return;
    Print("[Money] " + player.GetName() + " changed by " + amount +
          " (now has " + player.GetMoney() + ")");
}

void main()
{
    RegisterPlayerScript(PLAYER_ON_CHAT,       @OnChat);
    RegisterPlayerScript(PLAYER_ON_DUEL_END,   @OnDuelEnd);
    RegisterPlayerScript(PLAYER_ON_MONEY_CHANGE, @OnMoneyChange);
}
