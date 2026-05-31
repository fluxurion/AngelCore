/*
 * Character Services System
 * Name change, faction change, race change, guild services
 */

#include "../includes/ScriptFramework.as"
#include "BattlePayOpcodes.as"

// ============================================================================
// CHARACTER SERVICE PROCESSING
// Called from OnPlayerLogin for pending services
// ============================================================================
bool ProcessCharacterService(Player@ player, uint32 serviceType)
{
    if (player is null) return false;

    uint32 charGuid = player.GetGUIDLow();

    if (serviceType == PRODUCT_NAME_CHANGE)
    {
        // Set at_login flag for name change (bit 0 = AT_LOGIN_RENAME)
        CharacterExecute("UPDATE characters SET at_login = at_login | 1 WHERE guid = " + charGuid);
        player.SendNotification("Name change will be available on next login");
        Print(AS_COLOR_CYAN + "[BattlePay] Name change queued for character " + charGuid + AS_COLOR_RESET);
        return true;
    }
    else if (serviceType == PRODUCT_FACTION_CHANGE)
    {
        // Set at_login flag for faction change (bit 2 = AT_LOGIN_CHANGE_FACTION)
        CharacterExecute("UPDATE characters SET at_login = at_login | 4 WHERE guid = " + charGuid);
        player.SendNotification("Faction change will be available on next login");
        Print(AS_COLOR_CYAN + "[BattlePay] Faction change queued for character " + charGuid + AS_COLOR_RESET);
        return true;
    }
    else if (serviceType == PRODUCT_RACE_CHANGE)
    {
        // Set at_login flag for race change (bit 1 = AT_LOGIN_CHANGE_RACE)
        CharacterExecute("UPDATE characters SET at_login = at_login | 2 WHERE guid = " + charGuid);
        player.SendNotification("Race change will be available on next login");
        Print(AS_COLOR_CYAN + "[BattlePay] Race change queued for character " + charGuid + AS_COLOR_RESET);
        return true;
    }
    else if (serviceType == PRODUCT_CHAR_TRANSFER)
    {
        // Character transfer - queue for processing
        AngelDB_Execute(
            "INSERT INTO battlepay_character_transfers (character_guid, account_id, status, requested_at) VALUES ("
            + charGuid + ", " + player.GetAccountId() + ", 0, NOW()) "
            "ON DUPLICATE KEY UPDATE status = 0, requested_at = NOW()");
        player.SendNotification("Character transfer has been queued");
        Print(AS_COLOR_CYAN + "[BattlePay] Character transfer queued for character " + charGuid + AS_COLOR_RESET);
        return true;
    }
    else if (serviceType == PRODUCT_GUILD_NAME_CHANGE)
    {
        return ProcessGuildService(player, "namechange");
    }
    else if (serviceType == PRODUCT_GUILD_FACTION_CHANGE)
    {
        return ProcessGuildService(player, "factionchange");
    }
    else if (serviceType == PRODUCT_GUILD_TRANSFER)
    {
        return ProcessGuildService(player, "transfer");
    }

    return false;
}

// ============================================================================
// GUILD SERVICE PROCESSING
// ============================================================================
bool ProcessGuildService(Player@ player, string serviceType)
{
    if (player is null) return false;

    uint32 guildId = player.GetGuildId();
    if (guildId == 0)
    {
        player.SendNotification("You are not in a guild");
        return false;
    }

    // Verify player is guild master
    QueryResult@ result = CharacterQuery(
        "SELECT leaderguid FROM guild WHERE guildid = " + guildId);
    if (result is null || !result.NextRow())
    {
        player.SendNotification("Guild not found");
        return false;
    }

    uint64 leaderGuid = result.GetUInt64(0);
    if (leaderGuid != player.GetGUID())
    {
        player.SendNotification("Only the Guild Master can purchase guild services");
        return false;
    }

    // Queue guild service
    AngelDB_Execute(
        "INSERT INTO battlepay_guild_services (guild_id, service_type, account_id, status, requested_at) VALUES ("
        + guildId + ", '" + serviceType + "', " + player.GetAccountId() + ", 0, NOW()) "
        "ON DUPLICATE KEY UPDATE service_type = '" + serviceType + "', status = 0, requested_at = NOW()");

    if (serviceType == "namechange")
        player.SendNotification("Guild name change requested");
    else if (serviceType == "factionchange")
        player.SendNotification("Guild faction change requested");
    else if (serviceType == "transfer")
        player.SendNotification("Guild transfer requested");

    Print(AS_COLOR_CYAN + "[BattlePay] Guild service '" + serviceType + "' queued for guild " + guildId + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// CHECK PENDING SERVICES ON LOGIN
// ============================================================================
void CheckPendingServices(Player@ player)
{
    if (player is null) return;

    uint32 charGuid = player.GetGUIDLow();
    uint32 accountId = player.GetAccountId();

    // Check character at_login flags
    QueryResult@ result = CharacterQuery(
        "SELECT at_login FROM characters WHERE guid = " + charGuid);

    if (result !is null && result.NextRow())
    {
        uint32 atLogin = result.GetUInt32(0);

        if ((atLogin & 1) != 0)
            player.SendNotification("You have a pending name change! Use #bpay service namechange");
        if ((atLogin & 2) != 0)
            player.SendNotification("You have a pending race change! Use #bpay service racechange");
        if ((atLogin & 4) != 0)
            player.SendNotification("You have a pending faction change! Use #bpay service factionchange");
    }

    // Check pending guild services (if player is guild master)
    uint32 guildId = player.GetGuildId();
    if (guildId > 0)
    {
        AngelDBResult guildResult = AngelDB_Query(
            "SELECT service_type FROM battlepay_guild_services "
            "WHERE guild_id = " + guildId + " AND status = 0");

        if (guildResult.GetRowCount() > 0 && guildResult.NextRow())
        {
            string serviceType = guildResult.GetString(0);
            player.SendNotification("Your guild has a pending " + serviceType + " request");
        }
    }
}

// ============================================================================
// FORCE APPLY SERVICE (GM/Admin Command)
// ============================================================================
bool ForceApplyService(Player@ player, string serviceName)
{
    if (player is null) return false;

    uint32 charGuid = player.GetGUIDLow();
    uint32 serviceType = 0;

    if (serviceName == "namechange")
        serviceType = PRODUCT_NAME_CHANGE;
    else if (serviceName == "factionchange")
        serviceType = PRODUCT_FACTION_CHANGE;
    else if (serviceName == "racechange")
        serviceType = PRODUCT_RACE_CHANGE;
    else
        return false;

    // Check for pending reward
    AngelDBResult result = AngelDB_Query(
        "SELECT id FROM battlepay_pending_rewards "
        "WHERE account_id = " + player.GetAccountId() + " AND character_guid = " + charGuid +
        " AND reward_type = 'service' AND reward_id = " + serviceType + " AND status = 0 LIMIT 1");

    if (result.GetRowCount() > 0 && result.NextRow())
    {
        uint64 rewardId = result.GetUInt64(0);

        bool processed = ProcessCharacterService(player, serviceType);
        if (processed)
        {
            AngelDB_Execute("UPDATE battlepay_pending_rewards SET status = 1, delivered_at = NOW() WHERE id = " + rewardId);
            return true;
        }
    }

    // Try to apply directly even without pending reward
    return ProcessCharacterService(player, serviceType);
}
