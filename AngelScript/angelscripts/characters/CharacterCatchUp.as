/*
 * Character Catch-Up System
 * Provides gear appropriate for character level
 */

#include "../battlepay/BattlePayOpcodes.as"
#include "../battlepay/BattlePayStubs.as"
#include "../battlepay/BattlePayDelivery.as"

// ============================================================================
// CATCH-UP - Player Online
// ============================================================================
bool DeliverCatchUp(Player@ player, uint32 targetLevel)
{
    if (player is null) return false;

    uint32 charGuid = player.GetGUIDLow();
    uint32 currentLevel = player.GetLevel();

    if (targetLevel == 0)
        targetLevel = currentLevel;

    // Calculate appropriate gear level for target level
    uint32 avgItemLevel = CalculateCatchUpItemLevel(targetLevel);

    // Store catch-up request
    AngelDB_Execute(
        "INSERT INTO character_catchup_requests (guid, target_level, avg_item_level, status, requested_at) VALUES ("
        + charGuid + ", " + targetLevel + ", " + avgItemLevel + ", 0, NOW()) "
        "ON DUPLICATE KEY UPDATE target_level = " + targetLevel + ", avg_item_level = " + avgItemLevel + ", status = 0, requested_at = NOW()");

    // Set at_login flag to trigger catch-up on next login (bit 6)
    CharacterExecute("UPDATE characters SET at_login = at_login | 64 WHERE guid = " + charGuid);

    player.SendNotification("Catch-up gear requested for level " + targetLevel);
    Print(AS_COLOR_CYAN + "[BattlePay] Character " + charGuid + " requested catch-up to level " + targetLevel + " (ilvl " + avgItemLevel + ")" + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// CATCH-UP - Offline/Character GUID
// ============================================================================
bool DeliverCatchUp(uint32 characterGuid, uint32 targetLevel)
{
    if (characterGuid == 0) return false;

    if (targetLevel == 0)
    {
        // Get current level
        QueryResult@ result = CharacterQuery("SELECT level FROM characters WHERE guid = " + characterGuid);
        if (result !is null && result.NextRow())
            targetLevel = result.GetUInt32(0);
        else
            targetLevel = 70;
    }

    uint32 avgItemLevel = CalculateCatchUpItemLevel(targetLevel);

    // Queue catch-up for offline character
    AngelDB_Execute(
        "INSERT INTO character_catchup_requests (guid, target_level, avg_item_level, status, requested_at) VALUES ("
        + characterGuid + ", " + targetLevel + ", " + avgItemLevel + ", 0, NOW()) "
        "ON DUPLICATE KEY UPDATE target_level = " + targetLevel + ", avg_item_level = " + avgItemLevel + ", status = 0, requested_at = NOW()");

    // Set flag for next login
    CharacterExecute("UPDATE characters SET at_login = at_login | 64 WHERE guid = " + characterGuid);

    Print(AS_COLOR_CYAN + "[BattlePay] Queued catch-up for character " + characterGuid + " to level " + targetLevel + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// ITEM LEVEL CALCULATOR
// ============================================================================
uint32 CalculateCatchUpItemLevel(uint32 targetLevel)
{
    if (targetLevel >= 70)
        return 400; // Max level catch-up gear
    else if (targetLevel >= 60)
        return 200 + (targetLevel - 60) * 20;
    else if (targetLevel >= 50)
        return 100 + (targetLevel - 50) * 10;
    else
        return targetLevel * 3;
}

// ============================================================================
// LOGIN PROCESSING
// Called from OnPlayerLogin to process pending catch-up requests
// ============================================================================
void ProcessPendingCatchUp(Player@ player)
{
    if (player is null) return;

    uint32 charGuid = player.GetGUIDLow();

    AngelDBResult result = AngelDB_Query(
        "SELECT target_level, avg_item_level FROM character_catchup_requests "
        "WHERE guid = " + charGuid + " AND status = 0");

    if (result.GetRowCount() == 0 || !result.NextRow())
        return;

    uint32 targetLevel = result.GetUInt32(0);
    uint32 avgItemLevel = result.GetUInt32(1);

    // Generate catch-up gear via mail
    GenerateCatchUpGearMail(charGuid, targetLevel, avgItemLevel, player);

    // Mark as processed
    AngelDB_Execute("UPDATE character_catchup_requests SET status = 1, processed_at = NOW() WHERE guid = " + charGuid);

    player.SendNotification("Your catch-up gear has been sent to your mailbox!");
    Print(AS_COLOR_GREEN + "[BattlePay] Processed catch-up for character " + charGuid + AS_COLOR_RESET);
}

// ============================================================================
// GEAR MAIL GENERATOR
// ============================================================================
void GenerateCatchUpGearMail(uint32 characterGuid, uint32 targetLevel, uint32 avgItemLevel, Player@ player)
{
    string subject = "BattlePay Gear Update - Level " + targetLevel;
    string body = "Your requested catch-up gear for level " + targetLevel + " is attached.\n\nAverage Item Level: " + avgItemLevel;

    // Send gold for purchasing appropriate gear
    uint32 goldAmount = 0;
    if (targetLevel >= 70)
        goldAmount = 5000000; // 500 gold
    else if (targetLevel >= 60)
        goldAmount = 2000000; // 200 gold
    else
        goldAmount = targetLevel * 10000; // Scale with level

    // Use mail delivery from BattlePayDelivery module
    DeliverGoldViaMail(characterGuid, goldAmount, subject, body);

    // Also send some starter items based on class
    if (player !is null)
    {
        uint32 classId = player.GetClass();
        // Send consumables based on class
        SendCatchUpConsumables(characterGuid, classId, targetLevel);
    }
}

// ============================================================================
// CONSUMABLES FOR CATCH-UP
// ============================================================================
void SendCatchUpConsumables(uint32 characterGuid, uint32 classId, uint32 targetLevel)
{
    // Send appropriate consumables for the level
    uint32 itemId = 0;
    uint32 count = 10;

    if (targetLevel >= 60)
    {
        // High level consumables
        if (classId == 1 || classId == 2 || classId == 6 || classId == 10) // Tank/Melee
            itemId = 191380; // Example: high level healing potion
        else if (classId == 5 || classId == 8 || classId == 9 || classId == 11) // Caster
            itemId = 191381; // Example: high level mana potion
        else
            itemId = 191382; // Generic consumable
    }
    else if (targetLevel >= 40)
    {
        itemId = 171267; // Mid-level consumable
    }
    else
    {
        itemId = 118; // Low level consumable
    }

    if (itemId > 0)
    {
        DeliverViaMail(characterGuid, itemId, count,
            "Catch-Up Consumables",
            "Useful items to help you get started at your new level.");
    }
}

// ============================================================================
// BATTLEPAY INTEGRATION
// Called from battlepay.as when PRODUCT_GEAR_CATCHUP is purchased
// ============================================================================
bool ProcessGearCatchUpFromBattlePay(Player@ player, uint32 targetLevel)
{
    if (player is null)
    {
        Print(AS_COLOR_RED + "[BattlePay] Cannot deliver catch-up - player is null" + AS_COLOR_RESET);
        return false;
    }

    return DeliverCatchUp(player, targetLevel);
}

bool ProcessGearCatchUpFromBattlePay(uint32 characterGuid, uint32 targetLevel, uint32 accountId)
{
    if (characterGuid == 0)
    {
        Print(AS_COLOR_RED + "[BattlePay] Cannot queue catch-up - invalid GUID" + AS_COLOR_RESET);
        return false;
    }

    return DeliverCatchUp(characterGuid, targetLevel);
}
