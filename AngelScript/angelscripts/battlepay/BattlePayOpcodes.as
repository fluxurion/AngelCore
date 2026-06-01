/*
 * BattlePay Opcodes and Constants
 * Based on Opcodes.h from TrinityCore
 */

// ============================================================================
// BATTLEPAY OPCODES (CMSG - Client to Server)
// ============================================================================

const uint32 CMSG_BATTLE_PAY_ACK_FAILED_RESPONSE                     = 0x4000FC;
const uint32 CMSG_BATTLE_PAY_CANCEL_OPEN_CHECKOUT                    = 0x400141;
const uint32 CMSG_BATTLE_PAY_CONFIRM_PURCHASE_RESPONSE               = 0x4000FB;
const uint32 CMSG_BATTLE_PAY_DISTRIBUTION_ASSIGN_TO_TARGET           = 0x4000F2;
const uint32 CMSG_BATTLE_PAY_DISTRIBUTION_ASSIGN_VAS                 = 0x400167;
const uint32 CMSG_BATTLE_PAY_GET_PRODUCT_LIST                        = 0x4000E9;
const uint32 CMSG_BATTLE_PAY_GET_PURCHASE_LIST                       = 0x4000EA;
const uint32 CMSG_BATTLE_PAY_OPEN_CHECKOUT                           = 0x40013A;
const uint32 CMSG_BATTLE_PAY_REQUEST_PRICE_INFO                      = 0x400135;
const uint32 CMSG_BATTLE_PAY_START_PURCHASE                          = 0x4000FA;
const uint32 CMSG_BATTLE_PAY_START_VAS_PURCHASE                      = 0x400122;

// ============================================================================
// BATTLEPAY OPCODES (SMSG - Server to Client)
// ============================================================================

const uint32 SMSG_BATTLE_PAY_ACK_FAILED                              = 0x420233;
const uint32 SMSG_BATTLE_PAY_BATTLE_PET_DELIVERED                    = 0x420222;
const uint32 SMSG_BATTLE_PAY_COLLECTION_ITEM_DELIVERED               = 0x420223;
const uint32 SMSG_BATTLE_PAY_CONFIRM_PURCHASE                        = 0x420232;
const uint32 SMSG_BATTLE_PAY_DELIVERY_ENDED                          = 0x420220;
const uint32 SMSG_BATTLE_PAY_DELIVERY_STARTED                        = 0x42021F;
const uint32 SMSG_BATTLE_PAY_DISTRIBUTION_ASSIGN_VAS_RESPONSE        = 0x420316;
const uint32 SMSG_BATTLE_PAY_DISTRIBUTION_UNREVOKED                  = 0x42021D;
const uint32 SMSG_BATTLE_PAY_DISTRIBUTION_UPDATE                     = 0x42021E;
const uint32 SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE          = 0x42021C;
const uint32 SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE               = 0x42021A;
const uint32 SMSG_BATTLE_PAY_GET_PURCHASE_LIST_RESPONSE              = 0x42021B;
const uint32 SMSG_BATTLE_PAY_MOUNT_DELIVERED                       = 0x420221;
const uint32 SMSG_BATTLE_PAY_PURCHASE_UPDATE                         = 0x420231;
const uint32 SMSG_BATTLE_PAY_START_CHECKOUT                          = 0x4202CB;
const uint32 SMSG_BATTLE_PAY_START_DISTRIBUTION_ASSIGN_TO_TARGET_RESPONSE = 0x42022F;
const uint32 SMSG_BATTLE_PAY_START_PURCHASE_RESPONSE                 = 0x42022E;
const uint32 SMSG_BATTLE_PAY_VALIDATE_PURCHASE_RESPONSE              = 0x4202BE;
const uint32 SMSG_DISPLAY_PROMOTION                                  = 0x4200F3;

// ============================================================================
// ACCOUNT STORE OPCODES (for BattlePay currency/store)
// ============================================================================

const uint32 CMSG_ACCOUNT_STORE_BEGIN_PURCHASE_OR_REFUND             = 0x4000C1;
const uint32 SMSG_ACCOUNT_STORE_CURRENCY_UPDATE                      = 0x42032B;
const uint32 SMSG_ACCOUNT_STORE_FRONT_UPDATE                         = 0x42032C;
const uint32 SMSG_ACCOUNT_STORE_ITEM_STATE_CHANGED                   = 0x42032D;
const uint32 SMSG_ACCOUNT_STORE_RESULT                               = 0x42032E;

// ============================================================================
// ADDITIONAL STORE/VAS OPCODES
// ============================================================================

const uint32 CMSG_UPDATE_VAS_PURCHASE_STATES                         = 0x400123;
const uint32 CMSG_REQUEST_STORE_FRONT_INFO_UPDATE                    = 0x290023;
const uint32 CMSG_CATALOG_SHOP_LICENSE_GAME_DATA_REQUEST             = 0x4000FD;
const uint32 CMSG_SOCIAL_CONTRACT_REQUEST                            = 0x400176;
const uint32 CMSG_CAN_REDEEM_TOKEN_FOR_BALANCE                       = 0x400134;
const uint32 CMSG_GET_LAST_CATALOG_FETCH                           = 0x290036;
const uint32 CMSG_UPDATE_LAST_CATALOG_FETCH                        = 0x290035;
const uint32 SMSG_ENUM_VAS_PURCHASE_STATES_RESPONSE                  = 0x42029B;
const uint32 SMSG_SOCIAL_CONTRACT_REQUEST_RESPONSE                   = 0x420323;
const uint32 SMSG_LAST_CATALOG_FETCH_RESPONSE                        = 0x42037E;
const uint32 SMSG_CATALOG_SHOP_LICENSE_DATA                          = 0x4202BF;
const uint32 SMSG_CATALOG_SHOP_OBTAIN_LICENSE                        = 0x42036C;
const uint32 SMSG_SYNC_WOW_ENTITLEMENTS                              = 0x4202FC;

// ============================================================================
// ADDITIONAL UNDOCUMENTED SMSG OPCODES
// ============================================================================

const uint32 SMSG_BATTLE_PAY_CHARACTER_SERVICE_4218B4                = 0x4218B4;  // Unknown - sent after purchase list, contains character service data

// ============================================================================
// BATTLEPAY PRODUCT TYPE CONSTANTS
// ============================================================================

const uint32 PRODUCT_ITEM            = 1;
const uint32 PRODUCT_MOUNT           = 2;
const uint32 PRODUCT_BATTLE_PET      = 3;
const uint32 PRODUCT_TOY             = 4;
const uint32 PRODUCT_TRANSMOG        = 5;
const uint32 PRODUCT_LEVEL_BOOST     = 6;
const uint32 PRODUCT_GOLD            = 15;
const uint32 PRODUCT_GEAR_CATCHUP      = 7;
const uint32 PRODUCT_NAME_CHANGE       = 8;
const uint32 PRODUCT_FACTION_CHANGE  = 9;
const uint32 PRODUCT_RACE_CHANGE       = 10;
const uint32 PRODUCT_CHAR_TRANSFER     = 11;
const uint32 PRODUCT_GUILD_NAME_CHANGE       = 12;
const uint32 PRODUCT_GUILD_FACTION_CHANGE    = 13;
const uint32 PRODUCT_GUILD_TRANSFER          = 14;
