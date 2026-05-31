/*
 * BattlePay Packet Hooks
 * Hooks into AuthResponse and FeatureSystemStatusGlueScreen packets
 * to modify them with values from Config.as
 */

#include "../includes/ScriptFramework.as"
#include "../Config.as"
#include "BattlePayOpcodes.as"
#include "BattlePayPackets.as"

// Note: Hook funcdefs (AuthResponseHook, FeatureSystemStatusGlueScreenHook) are registered from C++

// ============================================================================
// PACKET HOOKS - C++ calls these functions before sending packets
// ============================================================================

// Hook: ON_AUTH_RESPONSE
// Modifies the CurrencyID in AuthSuccessInfo
// Called from C++: AngelScriptMgr::TriggerCustomHook_AuthResponse()
uint32 OnAuthResponse(uint32 sessionID, uint32 currencyID)
{
    Print(AS_COLOR_CYAN + "[BattlePay] AuthResponse hook - setting CurrencyID to: " + CONFIG_BPAY_STORE_CURRENCY + AS_COLOR_RESET);
    return CONFIG_BPAY_STORE_CURRENCY;
}

// Hook: ON_FEATURE_SYSTEM_STATUS_GLUE_SCREEN
// Modifies BpayStoreAvailable, ActiveBoostType, CommerceServerEnabled, CommercePricePollTimeSeconds, ContentSetID
// Called from C++: AngelScriptMgr::TriggerCustomHook_FeatureSystemStatusGlueScreen()
bool OnFeatureSystemStatusGlueScreen(uint32 sessionID, bool bpayStoreAvailable, int32 activeBoostType, int&out commerceServerEnabled, uint32&out commercePricePollTimeSeconds, int32&out contentSetID)
{
    commerceServerEnabled = CONFIG_BPAY_COMMERCE_SERVER_ENABLED ? 1 : 0;
    commercePricePollTimeSeconds = CONFIG_BPAY_COMMERCE_PRICE_POLL_SECONDS;
    contentSetID = CONFIG_BPAY_CONTENT_SET_ID;
    return CONFIG_BPAY_STORE_ENABLED;
}

// Hook: ON_FEATURE_SYSTEM_STATUS (ingame)
bool OnFeatureSystemStatus(uint32 sessionID, bool bpayStoreAvailable, int&out commerceServerEnabled, uint32&out commercePricePollTimeSeconds)
{
    commerceServerEnabled = CONFIG_BPAY_COMMERCE_SERVER_ENABLED ? 1 : 0;
    commercePricePollTimeSeconds = CONFIG_BPAY_COMMERCE_PRICE_POLL_SECONDS_INGAME;
    return CONFIG_BPAY_STORE_ENABLED;
}

// ============================================================================
// Hook: ON_SESSION_INITIALIZED
// Triggered at the end of WorldSession::InitializeSessionCallback,
// after all session initialization packets are sent and the player
// is on the character selection screen.
// Sends the Distribution List and Currency Update — these are required
// for C_StoreSecure.HasDistributionList() to pass on the client.
// ============================================================================
void OnSessionInitialized(WorldSession@ session)
{
    if (session is null)
        return;

    Print(AS_COLOR_CYAN + "[BattlePay] SessionInitialized hook - sending store init packets" + AS_COLOR_RESET);

    // SMSG_DISPLAY_PROMOTION - clear promotion popup
    SendPromotion(session);

    // SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE - required for HasDistributionList()
    SendDistributionList(session);

    // SMSG_ACCOUNT_STORE_CURRENCY_UPDATE - required for currency display
    SendCurrencyUpdate(session, CONFIG_BPAY_STORE_CURRENCY, 0);

    // SMSG_CATALOG_SHOP_OBTAIN_LICENSE - sent unsolicited at char screen on retail
    PacketData@ licPd = CreatePacketData(SMSG_CATALOG_SHOP_OBTAIN_LICENSE);
    licPd.WriteUInt32(0);  // LicenseID = 0 (no pending license)
    session.SendPacket(licPd);

    // SMSG_SYNC_WOW_ENTITLEMENTS - tells client about owned products. Empty = none owned.
    PacketData@ entPd = CreatePacketData(SMSG_SYNC_WOW_ENTITLEMENTS);
    entPd.WriteUInt32(0);  // PurchaseCountSize = 0
    entPd.WriteUInt32(0);  // ProductCount = 0
    session.SendPacket(entPd);
}

// ============================================================================
// HOOK REGISTRATION
// Called when BattlePay module loads
// ============================================================================
void RegisterBattlePayHooks()
{
    Print(AS_COLOR_CYAN + "[BattlePay] Registering packet hooks..." + AS_COLOR_RESET);

    // Register the hooks with AngelScript manager
    // These functions are registered by C++ API in ASWorldAPI.cpp
    // Use @ to get function handle
    RegisterAuthResponseHook(@OnAuthResponse);
    RegisterFeatureSystemStatusGlueScreenHook(@OnFeatureSystemStatusGlueScreen);
    RegisterFeatureSystemStatusHook(@OnFeatureSystemStatus);
    RegisterSessionInitializedHook(@OnSessionInitialized);

    Print(AS_COLOR_GREEN + "[BattlePay] Packet hooks registered successfully" + AS_COLOR_RESET);
}
