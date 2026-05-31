/*
 * BattlePay Configuration
 * Enums and result constants
 * Note: Opcodes are in BattlePayOpcodes.as
 */

// ============================================================================
// ENUMS
// ============================================================================
enum BattlePayResult
{
    BPAY_RESULT_SUCCESS             = 0,
    BPAY_RESULT_FAILED              = 1,
    BPAY_RESULT_INSUFFICIENT_FUNDS  = 2,
    BPAY_RESULT_ALREADY_OWNED       = 3,
    BPAY_RESULT_PRODUCT_DISABLED    = 4,
    BPAY_RESULT_INVALID_PRODUCT     = 5,
    BPAY_RESULT_INTERNAL_ERROR      = 6,
    BPAY_RESULT_NOT_AVAILABLE       = 7
}

enum ProductType
{
    PRODUCT_ITEM        = 0,
    PRODUCT_LEVEL_BOOST = 1,
    PRODUCT_PET         = 2,
    PRODUCT_MOUNT       = 3,
    PRODUCT_WOW_TOKEN   = 4,
    PRODUCT_NAME_CHANGE = 5,
    PRODUCT_FACTION_CHANGE = 6,
    PRODUCT_RACE_CHANGE = 8,
    PRODUCT_CHAR_TRANSFER = 11,
    PRODUCT_TOY         = 14,
    PRODUCT_EXPANSION   = 18,
    PRODUCT_GAME_TIME   = 20,
    PRODUCT_GUILD_NAME_CHANGE = 21,
    PRODUCT_GUILD_FACTION_CHANGE = 22,
    PRODUCT_GUILD_TRANSFER = 23,
    PRODUCT_TRANSMOG    = 26,
    PRODUCT_GOLD        = 30,
    PRODUCT_CURRENCY    = 31,
    PRODUCT_GEAR_CATCHUP = 32,  // CatchUpRequiredDays: Days of logout required for gear catch-up eligibility (0 = always available)
    PRODUCT_CUSTOM_ITEM_SET = 100,
    PRODUCT_CUSTOM_BUFF = 101,
    PRODUCT_CUSTOM_HEIRLOOM = 102,
    PRODUCT_CUSTOM_LOADOUT = 103,
    // Note: 104 was PRODUCT_CUSTOM_GEAR_UPDATE, now use PRODUCT_GEAR_CATCHUP (32)
    PRODUCT_CUSTOM_ACHIEVEMENT = 105,
    PRODUCT_CUSTOM_MULTI_QUEST = 106
}
