/*
 * AngelScript Global Configuration File
 * Contains configuration options for all AngelScript modules
 * Edit this file to customize script behavior without modifying core files
 */

// ============================================================================
// BATTLEPAY SHOP OPTIONS
// ============================================================================

// IngameShop.Enabled - Enable the shop in the charscreen and ingame
// Default: 0 (Disabled), 1 (Enabled)
bool CONFIG_BPAY_STORE_ENABLED = false;

// IngameShop.Currency - Payment currency ID
// 1=USD, 2=GBP, 3=BattleCoins, 4=EUR, 5=RUB, 8=ARS, 9=CLP, 10=MXN, 11=BRL, 
// 12=AUD, 14=CPT, 15=TPT, 16=BETA, 28=JPY, 29=CAD, 30=NZD
// Default: 3 (BattleCoins)
uint32 CONFIG_BPAY_STORE_CURRENCY = 3;

// Character.Upgrade.BoostType - Value sent in SendFeatureSystemStatusGlueScreen
// Default: 6 (Dragonflight level boost)
uint32 CONFIG_CHARACTER_UPGRADE_BOOST_TYPE = 6;

// CatchUpRequiredDays - Days of logout required for gear catch-up eligibility
// Set to 0 to allow anytime
// Default: 2
uint32 CONFIG_CATCH_UP_REQUIRED_DAYS = 2;
