/*
 * SaveAvgItemLevel.as
 *
 * Saves the player's average equipped item level to character_datas
 * on logout, so it can be sent in SMSG_ENUM_CHARACTERS_RESULT as
 * RegionwideCharacterInfo.AvgEquippedItemLevel.
 */
#include "../includes/Common.as"

const int ON_LOGOUT = 1; // PlayerHookType::ON_LOGOUT

void OnPlayerLogout(Player@ player)
{
    if (player is null)
        return;

    uint64 guidLow = player.GetGUIDLow();
    float avgItemLevel = player.GetAverageItemLevel();

    AngelDB_Execute(
        "INSERT INTO character_datas (guid, avgitemlevel) VALUES (" + guidLow + "," + avgItemLevel + ") "
        "ON DUPLICATE KEY UPDATE avgitemlevel = " + avgItemLevel
    );
}

void main()
{
    RegisterPlayerHook(ON_LOGOUT, @OnPlayerLogout);
    Print("[SaveAvgItemLevel] Registered ON_LOGOUT hook for avg item level persistence");
}
