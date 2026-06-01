/*
 * BattlePay Admin Commands
 * GM commands for managing BattlePay system
 */

#include "BattlePayDispatch.as"

// ============================================================================
// COMMAND HANDLER
// ============================================================================
bool HandleBattlePayCommand(Player@ player, string cmd, array<string>@ args)
{
    if (player is null) return false;
    if (!player.IsGM()) return false;

    if (cmd == "bpay" || cmd == "battlepay")
    {
        if (args.length() < 1)
        {
            player.SendNotification("Usage: #bpay credits|addcredits|product|reload|info|gear|service|upgrade [args]");
            return true;
        }

        string subcmd = args[0];

        if (subcmd == "credits" || subcmd == "balance")
        {
            uint32 accountID = player.GetAccountId();
            uint32 credits = GetAccountCredits(accountID);
            player.SendNotification("Your BattlePay credits: " + credits);
            return true;
        }
        else if (subcmd == "addcredits" || subcmd == "add")
        {
            if (args.length() < 2)
            {
                player.SendNotification("Usage: #bpay addcredits <amount> [player]");
                return true;
            }

            int amount = parseInt(args[1]);
            if (amount <= 0)
            {
                player.SendNotification("Invalid amount");
                return true;
            }

            Player@ target = player;
            if (args.length() >= 3)
            {
                @target = GetPlayerByName(args[2]);
                if (target is null)
                {
                    player.SendNotification("Player not found: " + args[2]);
                    return true;
                }
            }

            uint32 accountID = target.GetAccountId();
            AddCredits(accountID, amount);
            player.SendNotification("Added " + amount + " credits to " + target.GetName());
            target.SendNotification("You received " + amount + " BattlePay credits!");
            return true;
        }
        else if (subcmd == "product" || subcmd == "give")
        {
            if (args.length() < 2)
            {
                player.SendNotification("Usage: #bpay product <productID> [player]");
                return true;
            }

            uint32 productID = uint32(parseInt(args[1]));
            Player@ target = player;

            if (args.length() >= 3)
            {
                @target = GetPlayerByName(args[2]);
                if (target is null)
                {
                    player.SendNotification("Player not found: " + args[2]);
                    return true;
                }
            }

            ProductDataEntry@ prodData = FindProductData(productID);
            if (prodData is null)
            {
                player.SendNotification("Product not found: " + productID);
                return true;
            }

            bool delivered = DeliverProduct(target, prodData, 1);
            if (delivered)
            {
                player.SendNotification("Delivered product " + productID + " to " + target.GetName());
                target.SendNotification("You received a BattlePay product!");
            }
            else
            {
                player.SendNotification("Failed to deliver product " + productID);
            }
            return true;
        }
        else if (subcmd == "reload")
        {
            g_dataLoaded = false;
            g_disabledProducts.resize(0);
            g_groups.resize(0);
            g_shops.resize(0);
            g_productInfos.resize(0);
            g_productDatas.resize(0);
            g_productItems.resize(0);
            g_displayInfos.resize(0);

            LoadBattlePayData();
            if (player !is null)
                player.SendNotification("BattlePay data reloaded");
            else
                Print(AS_COLOR_CYAN + "[BattlePay] Data reloaded from console" + AS_COLOR_RESET);
            return true;
        }
        else if (subcmd == "info" || subcmd == "status")
        {
            player.SendNotification("BattlePay Status:");
            player.SendNotification("Groups: " + g_groups.length() + ", Shops: " + g_shops.length());
            player.SendNotification("Products: " + g_productDatas.length() + ", Disabled: " + g_disabledProducts.length());
            return true;
        }
        else if (subcmd == "upgrade")
        {
            Player@ target = player;

            if (args.length() >= 2)
            {
                @target = GetPlayerByName(args[1]);
                if (target is null)
                {
                    player.SendNotification("Player not found: " + args[1]);
                    return true;
                }
            }

            bool delivered = DeliverCharacterUpgradePackage(target, 0);
            if (delivered)
            {
                player.SendNotification("Character upgrade applied to " + target.GetName());
                target.SendNotification("Your character has been upgraded! Check your mail.");
            }
            else
            {
                player.SendNotification("Failed to apply character upgrade");
            }
            return true;
        }
        else if (subcmd == "service" || subcmd == "apply")
        {
            if (args.length() < 2)
            {
                player.SendNotification("Usage: #bpay service <namechange|factionchange|racechange>");
                return true;
            }

            string serviceType = args[1];

            bool processed = ForceApplyService(player, serviceType);
            if (processed)
            {
                player.SendNotification("Character service '" + serviceType + "' applied!");
            }
            else
            {
                player.SendNotification("Failed to apply character service '" + serviceType + "'");
            }
            return true;
        }
    }

    return false;
}
