/*
 * BattlePay Stubs - Placeholder implementations for missing functions
 * These provide minimal functionality to allow the scripts to compile
 */

#include "../includes/ScriptFramework.as"
#include "BattlePayData.as"
#include "BattlePayPackets.as"
#include "BattlePayOpcodes.as"

// ============================================================================
// CONFIG LOADER STUB
// ============================================================================

void LoadAngelScriptConfig()
{
    Print(AS_COLOR_CYAN + "[BattlePay] Config loaded (stub)" + AS_COLOR_RESET);
}

// ============================================================================
// PACKET HANDLER STUBS
// ============================================================================

bool HandleGetProductList(WorldSession@ session, PacketData@ packet)
{
    if (!g_dataLoaded)
        LoadBattlePayData();

    // Retail only responds with the product list here.
    // Promotion, distribution, and currency are sent once during OnSessionInitialized.
    // Purchase list is sent via its own CMSG_BATTLE_PAY_GET_PURCHASE_LIST handler.
    SendProductList(session, false);

    return true;
}

bool HandleGetPurchaseList(WorldSession@ session, PacketData@ packet)
{
    SendPurchaseList(session);
    return true;
}

bool HandleStartPurchase(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleStartPurchase called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleConfirmPurchaseResponse(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleConfirmPurchaseResponse called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleAckFailedResponse(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleAckFailedResponse called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleDistributionAssignToTarget(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleDistributionAssignToTarget called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleDistributionAssignVas(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleDistributionAssignVas called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleOpenCheckout(WorldSession@ session, PacketData@ packet)
{
    // Retail NEVER responds with SMSG_BATTLE_PAY_START_CHECKOUT.
    // Responding causes an infinite loop: client sends CMSG_BATTLE_PAY_OPEN_CHECKOUT
    // for every product, server responds, client sends next, etc.
    // Just absorb the request silently.
    return true;
}

bool HandleCancelOpenCheckout(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleCancelOpenCheckout called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleRequestPriceInfo(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleRequestPriceInfo called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleStartVasPurchase(WorldSession@ session, PacketData@ packet)
{
    Print(AS_COLOR_CYAN + "[BattlePay] HandleStartVasPurchase called (stub)" + AS_COLOR_RESET);
    return true;
}

bool HandleUpdateVasPurchaseStates(WorldSession@ session, PacketData@ packet)
{
    SendVasPurchaseStates(session);
    return true;
}

bool HandleRequestStoreFrontInfoUpdate(WorldSession@ session, PacketData@ packet)
{
    SendStoreFrontInfoUpdate(session);
    return true;
}

bool HandleCatalogShopLicenseGameDataRequest(WorldSession@ session, PacketData@ packet)
{
    // Client sends Count + [ID] array. Server responds with SMSG_CATALOG_SHOP_LICENSE_DATA.
    // Send empty response — no missing licenses.
    PacketData@ pd = CreatePacketData(SMSG_CATALOG_SHOP_LICENSE_DATA);
    pd.WriteUInt32(1);  // Field32 (retail = 1)
    pd.WriteUInt32(0);  // MissingLicenseGameDataCount = 0
    pd.WriteUInt32(0);  // Array2Count = 0
    pd.WriteUInt32(2);  // Field88 (retail = 2)
    session.SendPacket(pd);
    return true;
}

bool HandleSocialContractRequest(WorldSession@ session, PacketData@ packet)
{
    SendSocialContractResponse(session);
    return true;
}

bool HandleCanRedeemTokenForBalance(WorldSession@ session, PacketData@ packet)
{
    // Parser: ReadUInt32(Result), ReadUInt32(uint32_0), ReadUInt64(Balance), ReadUInt64(uint64_1), ReadByte(Byte)
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_VALIDATE_PURCHASE_RESPONSE);
    pd.WriteUInt32(0);   // Result = OK
    pd.WriteUInt32(CONFIG_BPAY_STORE_CURRENCY); // uint32_0
    pd.WriteUInt64(0);   // Balance
    pd.WriteUInt64(0);   // uint64_1
    pd.WriteUInt8(0);    // Byte (bit7=0)
    session.SendPacket(pd);
    return true;
}

bool HandleGetLastCatalogFetch(WorldSession@ session, PacketData@ packet)
{
    // Client asks for last catalog fetch timestamp. Respond with current time.
    // Retail: ReadUInt64("LastFetchTimestamp")
    PacketData@ pd = CreatePacketData(SMSG_LAST_CATALOG_FETCH_RESPONSE);
    pd.WriteUInt64(uint64(GetUnixTime()));  // LastFetchTimestamp — must be non-zero or client tries commerce server fetch that never completes
    session.SendPacket(pd);
    return true;
}

bool HandleUpdateLastCatalogFetch(WorldSession@ session, PacketData@ packet)
{
    // Client sending catalog fetch status update — just ack
    return true;
}

// ============================================================================
// DELIVERY STUBS - offline / SQL-based
// Note: DeliverViaMail/DeliverGoldViaMail are in BattlePayDelivery.as
// Note: DeliverCatchUp is in CharacterCatchUp.as
// ============================================================================

bool DeliverMount(uint32 characterGuid, uint32 spellID)
{
    if (characterGuid == 0 || spellID == 0) return false;
    QueryResult@ result = CharacterQuery(
        "SELECT spell FROM character_spell WHERE guid = " + characterGuid + " AND spell = " + spellID);
    if (result !is null && result.NextRow()) return false;
    CharacterExecute(
        "INSERT INTO character_spell (guid, spell, active, disabled) VALUES ("
        + characterGuid + ", " + spellID + ", 1, 0)");
    Print(AS_COLOR_GREEN + "[BattlePay] Added mount spell " + spellID + " to character " + characterGuid + AS_COLOR_RESET);
    return true;
}

bool DeliverToy(uint32 accountId, uint32 itemID)
{
    if (accountId == 0 || itemID == 0) return false;
    QueryResult@ result = LoginQuery(
        "SELECT itemId FROM account_toys WHERE battlenetAccountId = " + accountId + " AND itemId = " + itemID);
    if (result !is null && result.NextRow()) return false;
    LoginExecute(
        "INSERT INTO account_toys (battlenetAccountId, itemId, isFavourite) VALUES ("
        + accountId + ", " + itemID + ", 0)");
    Print(AS_COLOR_GREEN + "[BattlePay] Added toy " + itemID + " to account " + accountId + AS_COLOR_RESET);
    return true;
}

bool DeliverTransmogAppearance(uint32 accountId, uint32 transmogSetID)
{
    if (accountId == 0 || transmogSetID == 0) return false;
    QueryResult@ result = LoginQuery(
        "SELECT transmogSetId FROM battlenet_account_transmog_outfits WHERE battlenetAccountId = "
        + accountId + " AND transmogSetId = " + transmogSetID);
    if (result !is null && result.NextRow()) return true;
    LoginExecute(
        "INSERT INTO battlenet_account_transmog_outfits (battlenetAccountId, transmogSetId, name, icon, quality, "
        "unknown, type, unknown2, unknown3, unknown4, favorite) VALUES ("
        + accountId + ", " + transmogSetID + ", 'BattlePay Set', 0, 0, 0, 0, 0, 0, 0, 0)");
    Print(AS_COLOR_GREEN + "[BattlePay] Added transmog set " + transmogSetID + " to account " + accountId + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// CREDITS SYSTEM STUBS
// ============================================================================

void AddCredits(uint32 accountID, int64 amount)
{
    Print(AS_COLOR_CYAN + "[BattlePay] AddCredits: account=" + accountID + " amount=" + amount + " (stub)" + AS_COLOR_RESET);
}

int64 GetCredits(uint32 accountID)
{
    return 0;
}

// ============================================================================
// PRODUCT LOOKUP STUBS
// ============================================================================

ProductDataEntry@ FindProductData(uint32 productID)
{
    for (uint i = 0; i < g_productDatas.length(); i++)
    {
        if (g_productDatas[i].DeliverableID == productID)
            return g_productDatas[i];
    }
    return null;
}

ProductInfoEntry@ FindProductInfo(uint32 productID)
{
    for (uint i = 0; i < g_productInfos.length(); i++)
    {
        if (g_productInfos[i].ShopListingID == productID)
            return g_productInfos[i];
    }
    return null;
}

// ============================================================================
// SESSION HELPERS
// ============================================================================

WorldSession@ GetSession(uint32 accountID)
{
    // Return null - would need player lookup by account
    return null;
}

// ============================================================================
// DELIVERY PRODUCT DISPATCH STUBS
// ============================================================================

bool DeliverItem(Player@ player, uint32 itemID, uint32 count)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverItem: player=" + player.GetName() + " item=" + itemID + " count=" + count + " (stub)" + AS_COLOR_RESET);
    return player.AddItem(itemID, count);
}

bool DeliverMount(Player@ player, uint32 spellID)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverMount: player=" + player.GetName() + " spell=" + spellID + " (stub)" + AS_COLOR_RESET);
    return true;
}

bool DeliverBattlePet(Player@ player, uint32 speciesID)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverBattlePet: player=" + player.GetName() + " species=" + speciesID + " (stub)" + AS_COLOR_RESET);
    return true;
}

bool DeliverToy(Player@ player, uint32 itemID)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverToy: player=" + player.GetName() + " item=" + itemID + " (stub)" + AS_COLOR_RESET);
    return true;
}

bool DeliverTransmogAppearance(Player@ player, uint32 itemModifiedAppearanceID)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverTransmog: player=" + player.GetName() + " appearance=" + itemModifiedAppearanceID + " (stub)" + AS_COLOR_RESET);
    return true;
}

bool DeliverGold(Player@ player, uint64 amount)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverGold: player=" + player.GetName() + " amount=" + amount + " (stub)" + AS_COLOR_RESET);
    player.AddMoney(amount);
    return true;
}

bool DeliverLevelBoost(Player@ player, uint32 targetLevel, uint32 boostType)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverLevelBoost: player=" + player.GetName() + " targetLevel=" + targetLevel + " (stub)" + AS_COLOR_RESET);
    return true;
}

bool DeliverCharacterServices(Player@ player, uint32 serviceType, uint32 targetCharGuid)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverCharacterServices: player=" + player.GetName() + " service=" + serviceType + " (stub)" + AS_COLOR_RESET);
    return true;
}

bool DeliverGuildServices(Player@ player, uint32 serviceType)
{
    if (player is null) return false;
    Print(AS_COLOR_CYAN + "[BattlePay] DeliverGuildServices: player=" + player.GetName() + " service=" + serviceType + " (stub)" + AS_COLOR_RESET);
    return true;
}

// ============================================================================
// DATABASE EXECUTE STUB
// ============================================================================

void CharacterExecute(string sql)
{
    Print(AS_COLOR_CYAN + "[BattlePay] CharacterExecute (stub): " + sql.substr(0, 50) + "..." + AS_COLOR_RESET);
}
