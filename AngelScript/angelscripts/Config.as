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
bool CONFIG_BPAY_STORE_ENABLED = true;

// IngameShop.Currency - Payment currency ID
// 1=USD, 2=GBP, 3=BattleCoins, 4=EUR, 5=RUB, 8=ARS, 9=CLP, 10=MXN, 11=BRL,
// 12=AUD, 14=CPT, 15=TPT, 16=BETA, 28=JPY, 29=CAD, 30=NZD
// Default: 3 (BattleCoins)
uint32 CONFIG_BPAY_STORE_CURRENCY = 3;

// Character.Upgrade.BoostType - Value sent in SendFeatureSystemStatusGlueScreen
// Default: 6 (Dragonflight level boost)
uint32 CONFIG_CHARACTER_UPGRADE_BOOST_TYPE = 6;

// CommerceServerEnabled - Enable commerce server for shop functionality
// Required for C_CatalogShop fetch to succeed
// Default: true
bool CONFIG_BPAY_COMMERCE_SERVER_ENABLED = true;

// CommercePricePollTimeSeconds - Price polling interval in seconds
// Retail uses 60000 (16.6 hours) on glue screen, 300 (5 minutes) in-game
// Default: 60000 (glue screen), 300 (in-world)
uint32 CONFIG_BPAY_COMMERCE_PRICE_POLL_SECONDS = 60000;
uint32 CONFIG_BPAY_COMMERCE_PRICE_POLL_SECONDS_INGAME = 300;

// ContentSetID - Currently active Classic season/expansion
// Should match CONFIG_EXPANSION from world config
// Default: 11 (The War Within)
int32 CONFIG_BPAY_CONTENT_SET_ID = 11;

// CatchUpRequiredDays - Days of logout required for gear catch-up eligibility
// Set to 0 to allow anytime
// Default: 2
uint32 CONFIG_CATCH_UP_REQUIRED_DAYS = 2;

// ============================================================================
// ANGELDB - Independent MySQL Database for AngelScript
// Credentials are read from worldserver.conf (WorldDatabaseInfo).
// Only the database name can be overridden here if you need a custom DB.
// The C++ layer auto-creates the database if it doesn't exist.
// ============================================================================

// AngelDB.Database - Database name (optional, default: "angelcore_db")
string CONFIG_ANGELDB_DATABASE = "angelcore_db";

// AngelDB.UpdatesDir - Path to SQL update files (relative to angelscripts/ or absolute)
// The C++ layer scans <UpdatesDir>/pending/ for .sql files on startup/reload.
// Executed files are moved to <UpdatesDir>/applied/.
// Default: "AngelDB" (resolves to angelscripts/AngelDB/)
string CONFIG_ANGELDB_UPDATES_DIR = "AngelDB";

// ============================================================================
// CHARACTER ENUMERATION OPTIONS
// ============================================================================

// CharEnum.Realmless - Enable regionwide (realmless) character enumeration
// true: Sends regionwide character data with money/restrictions/mail support
// false: Sends basic character data (legacy mode)
// Default: true (regionwide)
bool CONFIG_CHAR_ENUM_REALMLESS = true;
