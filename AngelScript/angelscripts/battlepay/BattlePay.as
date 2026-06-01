/*
 * BattlePay System - AngelScript Implementation
 * Complete in-game shop with product listing, purchase flow, and delivery.
 * Modular architecture for better maintainability.
 */

#include "../includes/ScriptFramework.as"
#include "../includes/Common.as"

// Include global AngelScript config (contains all AS-specific settings)
#include "../Config.as"

// Include all BattlePay modules
#include "BattlePayEnums.as"
#include "BattlePayDispatch.as"
#include "BattlePayPackets.as"
#include "BattlePayCommands.as"
#include "BattlePayHooks.as"

// ============================================================================
// INITIALIZATION AND HOOKS
// ============================================================================

void OnStartup()
{
    LoadAngelScriptConfig();
    RegisterBattlePayHooks();
    LoadBattlePayData();
}

void OnPlayerLogin(Player@ player)
{
    if (player is null) return;

    uint32 accountID = player.GetAccountId();
    uint32 characterGuid = player.GetGUIDLow();

    // Check for pending distributions
    AngelDBResult result = AngelDB_Query(
        "SELECT d.id, d.purchase_id, d.product_id, d.status "
        "FROM battlepay_distributions d "
        "JOIN battlepay_purchases p ON d.purchase_id = p.id "
        "WHERE p.account_id = " + accountID + " AND d.status = 0");

    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            uint64 distributionID = result.GetUInt64(0);
            uint64 purchaseID = result.GetUInt64(1);
            uint32 productID = result.GetUInt32(2);

            for (uint i = 0; i < g_productDatas.length(); i++)
            {
                if (g_productDatas[i].DeliverableID == productID)
                {
                    DeliverProduct(player, g_productDatas[i], 1);
                    AngelDB_Execute("UPDATE battlepay_distributions SET status = 1 WHERE id = " + distributionID);
                    break;
                }
            }
        }
    }

    // Check for pending rewards (mail/SQL queued items)
    result = AngelDB_Query(
        "SELECT id, reward_type, reward_id, reward_amount "
        "FROM battlepay_pending_rewards "
        "WHERE account_id = " + accountID + " AND character_guid = " + characterGuid +
        " AND status = 0");

    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            uint64 rewardId = result.GetUInt64(0);
            string rewardType = result.GetString(1);
            uint32 rewardIdValue = result.GetUInt32(2);
            uint32 rewardAmount = result.GetUInt32(3);

            bool delivered = false;

            if (rewardType == 'item')
            {
                delivered = DeliverItem(player, rewardIdValue, rewardAmount);
                if (!delivered)
                {
                    delivered = DeliverViaMail(characterGuid, rewardIdValue, rewardAmount,
                        "BattlePay Delivery", "Your pending BattlePay item reward is attached.");
                }
            }
            else if (rewardType == 'mount')
            {
                delivered = DeliverMount(player, rewardIdValue);
                if (!delivered)
                    delivered = DeliverMount(characterGuid, rewardIdValue);
            }
            else if (rewardType == 'gold')
            {
                delivered = DeliverGold(player, rewardAmount);
            }
            else if (rewardType == 'service')
            {
                delivered = ProcessCharacterService(player, rewardIdValue);
            }

            if (delivered)
            {
                AngelDB_Execute("UPDATE battlepay_pending_rewards SET status = 1, delivered_at = NOW() WHERE id = " + rewardId);
                Print("[BattlePay] Reward " + rewardId + " delivered to " + player.GetName());
            }
            else
            {
                AngelDB_Execute("UPDATE battlepay_pending_rewards SET status = 2 WHERE id = " + rewardId);
                PrintError("[BattlePay] Reward " + rewardId + " delivery FAILED for " + player.GetName());
            }
        }
    }

    // Check for pending catch-up requests
    ProcessPendingCatchUp(player);

    // Check for pending character services
    CheckPendingServices(player);
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

void main()
{
    // Register all BattlePay opcode handlers
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_GET_PRODUCT_LIST, @HandleGetProductList, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_GET_PURCHASE_LIST, @HandleGetPurchaseList, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_START_PURCHASE, @HandleStartPurchase, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE, @HandleConfirmPurchaseResponse, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_ACK_FAILED_RESPONSE, @HandleAckFailedResponse, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_DISTRIBUTION_ASSIGN_TO_TARGET, @HandleDistributionAssignToTarget, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_DISTRIBUTION_ASSIGN_VAS, @HandleDistributionAssignVas, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_OPEN_CHECKOUT, @HandleOpenCheckout, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_CANCEL_OPEN_CHECKOUT, @HandleCancelOpenCheckout, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_REQUEST_PRICE_INFO, @HandleRequestPriceInfo, true);
    RegisterOpcodeHandler(CMSG_BATTLE_PAY_START_VAS_PURCHASE, @HandleStartVasPurchase, true);

    // Additional handlers needed for store to fully initialize
    RegisterOpcodeHandler(CMSG_UPDATE_VAS_PURCHASE_STATES, @HandleUpdateVasPurchaseStates, true);
    RegisterOpcodeHandler(CMSG_REQUEST_STORE_FRONT_INFO_UPDATE, @HandleRequestStoreFrontInfoUpdate, true);
    RegisterOpcodeHandler(CMSG_CATALOG_SHOP_LICENSE_GAME_DATA_REQUEST, @HandleCatalogShopLicenseGameDataRequest, true);
    RegisterOpcodeHandler(CMSG_SOCIAL_CONTRACT_REQUEST, @HandleSocialContractRequest, true);
    RegisterOpcodeHandler(CMSG_CAN_REDEEM_TOKEN_FOR_BALANCE, @HandleCanRedeemTokenForBalance, true);
    RegisterOpcodeHandler(CMSG_GET_LAST_CATALOG_FETCH, @HandleGetLastCatalogFetch, false);
    RegisterOpcodeHandler(CMSG_UPDATE_LAST_CATALOG_FETCH, @HandleUpdateLastCatalogFetch, false);

    // Decor/collection system handlers (stubs for now)
    RegisterOpcodeHandler(CMSG_GET_DECOR_REFUND_LIST, @HandleGetDecorRefundList, false);
    RegisterOpcodeHandler(CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES, @HandleGetAllLicensedDecorQuantities, false);

    // Commerce token handlers
    RegisterOpcodeHandler(CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE, @HandleCommerceTokenGetMarketPrice, false);
    RegisterOpcodeHandler(CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY, @HandleConsumableTokenCanVeteranBuy, false);

    // Register world hooks
    RegisterPlayerScript(PLAYER_ON_LOGIN, @OnPlayerLogin);

    // Run initialization immediately (also covers rel as reloads)
    OnStartup();

    Print("[BattlePay] Initialized");
}
