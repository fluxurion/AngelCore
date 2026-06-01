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
    return CONFIG_BPAY_STORE_CURRENCY;
}

// Hook: ON_FEATURE_SYSTEM_STATUS_GLUE_SCREEN
// Modifies BpayStoreAvailable, ActiveBoostType, CommerceServerEnabled, CommercePricePollTimeSeconds, ContentSetID
// Called from C++: AngelScriptMgr::TriggerCustomHook_FeatureSystemStatusGlueScreen()
bool OnFeatureSystemStatusGlueScreen(uint32 sessionID, bool bpayStoreAvailable, int32 activeBoostType)
{
    return CONFIG_BPAY_STORE_ENABLED;
}

bool OnFeatureSystemStatus(uint32 sessionID, bool bpayStoreAvailable)
{
    return CONFIG_BPAY_STORE_ENABLED;
}

// ============================================================================
// Hook: ON_SESSION_INITIALIZED
// Triggered at the end of WorldSession::InitializeSessionCallback,
// after all session initialization packets are sent and the player
// is on the character selection screen.
// Sends the Distribution List and Currency Update — these are required
// for C_StoreSecure.HasDistributionList() to pass on the client.
//
// REFACTORED: Packet order matches retail log:
//   1. SMSG_UNKNOWN_BEFORE_CHAR_ENUM (0x420224) - contains Guid + DisplayCard
//   2. SMSG_DISPLAY_PROMOTION
//   3. SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE
//   4. SMSG_ACCOUNT_STORE_CURRENCY_UPDATE
//   5. SMSG_CATALOG_SHOP_OBTAIN_LICENSE
//   6. SMSG_SYNC_WOW_ENTITLEMENTS
// ============================================================================
void OnSessionInitialized(WorldSession@ session)
{
    if (session is null)
        return;

    // SMSG_UNKNOWN_BEFORE_CHAR_ENUM (0x420224) - sent before/around char enum
    // Contains PackedGuid128 + DisplayCard. Using session's account GUID.
    SendUnknownBeforeCharEnum(session, session.GetAccountId());

    // SMSG_DISPLAY_PROMOTION - clear promotion popup
    SendPromotion(session);

    // SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE - required for HasDistributionList()
    SendDistributionList(session);

    // SMSG_ACCOUNT_STORE_CURRENCY_UPDATE - required for currency display
    SendCurrencyUpdate(session, CONFIG_BPAY_STORE_CURRENCY, 0);

    // SMSG_CATALOG_SHOP_OBTAIN_LICENSE - sent unsolicited at char screen on retail
    PacketData@ licPd = CreatePacketData(SMSG_CATALOG_SHOP_OBTAIN_LICENSE);
    licPd.WriteUInt32(1156753);  // LicenseID = 0 (no pending license)
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
    // Register the hooks with AngelScript manager
    // These functions are registered by C++ API in ASWorldAPI.cpp
    // Use @ to get function handle
    RegisterAuthResponseHook(@OnAuthResponse);
    RegisterFeatureSystemStatusGlueScreenHook(@OnFeatureSystemStatusGlueScreen);
    RegisterFeatureSystemStatusHook(@OnFeatureSystemStatus);
    RegisterSessionInitializedHook(@OnSessionInitialized);

}
