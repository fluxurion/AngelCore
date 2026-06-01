/*
 * BattlePay Dispatch - Routes product delivery to the correct subsystem
 * Include this file wherever DeliverProduct() is needed.
 */

#include "../includes/ScriptFramework.as"
#include "../Config.as"
#include "BattlePayOpcodes.as"
#include "BattlePayData.as"
#include "BattlePayStubs.as"
#include "BattlePayDelivery.as"
#include "../characters/CharacterUpgrade.as"
#include "CharacterServices.as"

// ============================================================================
// DELIVERY PRODUCT DISPATCH
// Main entry point for product delivery - routes to appropriate module
// ============================================================================

bool DeliverProduct(Player@ player, const ProductDataEntry& productData, uint32 count, uint32 characterGuid = 0)
{
    uint32 accountID = player.GetAccountId();
    uint32 targetGuid = characterGuid;

    if (targetGuid == 0 && player !is null)
        targetGuid = player.GetGUIDLow();

    uint32 type = productData.Type;
    bool delivered = false;

    // Try direct delivery if player is online
    if (player !is null)
    {
        if (type == PRODUCT_ITEM)
        {
            delivered = DeliverItem(player, productData.ItemID, productData.ItemCount * count);
            if (!delivered)
            {
                Print(AS_COLOR_YELLOW + "[BattlePay] Inventory full, falling back to mail delivery" + AS_COLOR_RESET);
                delivered = DeliverViaMail(targetGuid, productData.ItemID, productData.ItemCount * count,
                    "BattlePay Purchase", "Thank you for your purchase!");
            }
        }
        else if (type == PRODUCT_MOUNT)
        {
            delivered = DeliverMount(player, productData.MountSpellID);
            if (!delivered)
                delivered = DeliverMount(targetGuid, productData.MountSpellID);
        }
        else if (type == PRODUCT_BATTLE_PET)
        {
            delivered = DeliverBattlePet(player, productData.BattlePetSpeciesCreatureID);
        }
        else if (type == PRODUCT_TOY)
        {
            delivered = DeliverToy(player, productData.ItemID);
            if (!delivered)
                delivered = DeliverToy(accountID, productData.ItemID);
        }
        else if (type == PRODUCT_GOLD)
        {
            delivered = DeliverGold(player, productData.ItemCount * count);
        }
        else if (type == PRODUCT_LEVEL_BOOST)
        {
            delivered = ProcessCharacterUpgradeFromBattlePay(player, productData.BoostType);
        }
        else if (type == PRODUCT_TRANSMOG)
        {
            delivered = DeliverTransmogAppearance(player, productData.TransmogSetID);
            if (!delivered)
                delivered = DeliverTransmogAppearance(accountID, productData.TransmogSetID);
        }
        else if (type == PRODUCT_NAME_CHANGE || type == PRODUCT_FACTION_CHANGE ||
                 type == PRODUCT_RACE_CHANGE || type == PRODUCT_CHAR_TRANSFER ||
                 type == PRODUCT_GUILD_NAME_CHANGE || type == PRODUCT_GUILD_FACTION_CHANGE ||
                 type == PRODUCT_GUILD_TRANSFER)
        {
            delivered = ProcessCharacterService(player, type);
        }
        else
        {
            Print(AS_COLOR_RED + "[BattlePay] Unknown product type " + type + " for product " + productData.DeliverableID + AS_COLOR_RESET);
            delivered = false;
        }
    }
    else
    {
        // Player is offline / on character selection - use mail or SQL delivery
        Print(AS_COLOR_CYAN + "[BattlePay] Player offline, using mail/SQL delivery for type " + type + AS_COLOR_RESET);

        if (type == PRODUCT_ITEM)
        {
            delivered = DeliverViaMail(targetGuid, productData.ItemID, productData.ItemCount * count,
                "BattlePay Purchase", "Thank you for your purchase!");
        }
        else if (type == PRODUCT_GOLD)
        {
            delivered = DeliverGoldViaMail(targetGuid, uint64(productData.ItemCount * count),
                "BattlePay Gold Delivery", "Your gold purchase is attached.");
        }
        else if (type == PRODUCT_MOUNT)
        {
            delivered = DeliverMount(targetGuid, productData.MountSpellID);
        }
        else if (type == PRODUCT_TOY)
        {
            delivered = DeliverToy(accountID, productData.ItemID);
        }
        else if (type == PRODUCT_TRANSMOG)
        {
            delivered = DeliverTransmogAppearance(accountID, productData.TransmogSetID);
        }
        else if (type == PRODUCT_LEVEL_BOOST)
        {
            delivered = ProcessCharacterUpgradeFromBattlePay(targetGuid, productData.BoostType, accountID);
        }
        else if (type == PRODUCT_NAME_CHANGE || type == PRODUCT_FACTION_CHANGE ||
                 type == PRODUCT_RACE_CHANGE || type == PRODUCT_CHAR_TRANSFER ||
                 type == PRODUCT_GUILD_NAME_CHANGE || type == PRODUCT_GUILD_FACTION_CHANGE ||
                 type == PRODUCT_GUILD_TRANSFER)
        {
            AngelDB_Execute(
                "INSERT INTO battlepay_pending_rewards (account_id, character_guid, reward_type, reward_id, status) VALUES ("
                + accountID + ", " + targetGuid + ", 'service', " + type + ", 0)");
            delivered = true;
        }
        else
        {
            if (productData.ItemID > 0)
            {
                delivered = DeliverViaMail(targetGuid, productData.ItemID, productData.ItemCount * count,
                    "BattlePay Purchase", "Thank you for your purchase!");
            }
            else
            {
                Print(AS_COLOR_RED + "[BattlePay] No delivery method for offline player, type " + type + AS_COLOR_RESET);
                delivered = false;
            }
        }
    }

    return delivered;
}
