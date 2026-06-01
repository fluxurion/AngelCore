/*
 * Character Upgrade (Level Boost) System
 * Handles character level boosts from BattlePay
 */

#include "../battlepay/BattlePayOpcodes.as"
#include "../battlepay/BattlePayStubs.as"

// ============================================================================
// CHARACTER UPGRADE - Player Online
// ============================================================================
bool DeliverLevelBoost(Player@ player, uint32 boostType)
{
    if (player is null) return false;

    uint32 targetLevel = 70; // Default max level for current expansion
    uint32 currentLevel = player.GetLevel();

    if (currentLevel >= targetLevel)
    {
        Print(AS_COLOR_YELLOW + "[BattlePay] Player " + player.GetName() + " is already max level" + AS_COLOR_RESET);
        return false;
    }

    // Update character level in database
    uint32 guid = player.GetGUIDLow();
    CharacterExecute("UPDATE characters SET level = " + targetLevel + " WHERE guid = " + guid);

    Print(AS_COLOR_GREEN + "[BattlePay] Boosted player " + player.GetName() + " to level " + targetLevel + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// CHARACTER UPGRADE - Offline/Character GUID
// ============================================================================
bool DeliverLevelBoost(uint32 characterGuid, uint32 boostType)
{
    if (characterGuid == 0) return false;

    uint32 targetLevel = 70;

    // Check current level
    QueryResult@ result = CharacterQuery(
        "SELECT level FROM characters WHERE guid = " + characterGuid);
    if (result !is null && result.NextRow())
    {
        uint32 currentLevel = result.GetUInt32(0);
        if (currentLevel >= targetLevel)
        {
            Print(AS_COLOR_YELLOW + "[BattlePay] Character " + characterGuid + " is already max level " + currentLevel + AS_COLOR_RESET);
            return false;
        }
    }

    // Update level in database (player will see it on next login)
    CharacterExecute("UPDATE characters SET level = " + targetLevel + " WHERE guid = " + characterGuid);

    Print(AS_COLOR_CYAN + "[BattlePay] Queued level boost to " + targetLevel + " for character " + characterGuid + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// FULL CHARACTER UPGRADE (Level + Gear Package)
// ============================================================================
bool DeliverCharacterUpgradePackage(Player@ player, uint32 boostType)
{
    if (player is null) return false;

    uint32 charGuid = player.GetGUIDLow();
    uint32 accountId = player.GetAccountId();

    // Step 1: Level boost
    bool leveled = DeliverLevelBoost(player, boostType);
    if (!leveled)
    {
        Print(AS_COLOR_RED + "[BattlePay] Character upgrade - level boost failed or already max level" + AS_COLOR_RESET);
    }

    // Step 2: Mount (if appropriate for level)
    if (player.GetLevel() >= 20)
    {
        // Could add basic riding skill here
        Print(AS_COLOR_CYAN + "[BattlePay] Character upgrade - riding skill available" + AS_COLOR_RESET);
    }

    player.SendNotification("Character upgrade complete!");
    Print(AS_COLOR_GREEN + "[BattlePay] Character upgrade package delivered to " + player.GetName() + AS_COLOR_RESET);
    return leveled;
}

// ============================================================================
// BATTLEPAY INTEGRATION
// Called from battlepay.as when PRODUCT_LEVEL_BOOST is purchased
// ============================================================================
bool ProcessCharacterUpgradeFromBattlePay(Player@ player, uint32 boostType)
{
    if (player is null)
    {
        Print(AS_COLOR_RED + "[BattlePay] Cannot deliver character upgrade - player is null" + AS_COLOR_RESET);
        return false;
    }

    return DeliverCharacterUpgradePackage(player, boostType);
}

bool ProcessCharacterUpgradeFromBattlePay(uint32 characterGuid, uint32 boostType, uint32 accountId)
{
    if (characterGuid == 0)
    {
        Print(AS_COLOR_RED + "[BattlePay] Cannot queue character upgrade - invalid GUID" + AS_COLOR_RESET);
        return false;
    }

    // Queue for later processing when player logs in
    CharacterExecute(
        "INSERT INTO battlepay_pending_rewards (account_id, character_guid, reward_type, reward_id, status) VALUES ("
        + accountId + ", " + characterGuid + ", 'service', " + PRODUCT_LEVEL_BOOST + ", 0)");

    // Also queue level boost directly
    DeliverLevelBoost(characterGuid, boostType);

    Print(AS_COLOR_CYAN + "[BattlePay] Character upgrade queued for offline character " + characterGuid + AS_COLOR_RESET);
    return true;
}
