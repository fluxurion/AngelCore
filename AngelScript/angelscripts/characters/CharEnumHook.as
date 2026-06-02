/*
 * CharEnumHook.as
 * AngelScript hooks for SMSG_ENUM_CHARACTERS_RESULT
 *
 * OnCharEnum (PRE-send):  Copies all CharacterInfoBasic data from TC's
 *                          Characters into RegionwideCharacters, clears
 *                          the Characters list, then enriches with money.
 * OnPostCharEnum (POST-send): Sends SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
 *                             after SMSG_ENUM_CHARACTERS_RESULT.
 */
#include "../Config.as"
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

// ============================================================================
// Check if character is eligible for RPE/catchup
// ============================================================================
bool IsRPEEligible(uint64 characterGuid)
{
    // Query logout_time from characters table
    string query = "SELECT logout_time FROM characters WHERE guid = " + characterGuid;
    QueryResult@ result = CharacterQuery(query);

    if (result is null || result.GetRowCount() == 0)
        return false;

    uint64 logoutTime = result.GetUInt64(0);

    // Check for new character (never logged in or logout_time = 0)
    if (logoutTime == 0)
    {
        Print("[CharEnumHook] Character " + characterGuid + " has no logout_time (new character)");
        return CONFIG_RPE_ALLOW_FIRST_LOGIN;
    }

    // Calculate days since logout
    // logout_time is Unix timestamp in seconds
    uint64 currentTime = GetUnixTime();
    uint64 secondsSinceLogout = currentTime - logoutTime;
    uint64 daysSinceLogout = secondsSinceLogout / 86400;  // 86400 seconds in a day

    Print("[CharEnumHook] Character " + characterGuid + " logged out " + daysSinceLogout + " days ago");

    // Check if enough time has passed
    if (daysSinceLogout >= CONFIG_RPE_REQUIRED_LOGOUT_DAYS)
    {
        Print("[CharEnumHook] Character " + characterGuid + " is eligible for RPE (logout " + daysSinceLogout + " days ago, required: " + CONFIG_RPE_REQUIRED_LOGOUT_DAYS + ")");
        return true;
    }

    Print("[CharEnumHook] Character " + characterGuid + " is NOT eligible for RPE (logout " + daysSinceLogout + " days ago, required: " + CONFIG_RPE_REQUIRED_LOGOUT_DAYS + ")");
    return false;
}

void OnCharEnum(WorldSession@ session, EnumCharactersResult@ result)
{
    if (result is null)
    {
        Print("[CharEnumHook] ERROR: result is null!");
        return;
    }

    uint32 charCount = result.GetCharacterCount();
    Print("[CharEnumHook] Called - CharacterCount: " + charCount + ", Realmless: " + CONFIG_CHAR_ENUM_REALMLESS);

    // Set the Realmless flag in the packet based on config
    result.SetRealmless(CONFIG_CHAR_ENUM_REALMLESS);

    if (charCount == 0)
        return;

    // If realmless is disabled, use basic character data (legacy mode)
    if (!CONFIG_CHAR_ENUM_REALMLESS)
    {
        Print("[CharEnumHook] Realmless disabled - using basic character data");
        return;
    }

    // Move TC's full CharacterInfoBasic data into RegionwideCharacters
    // (copies VisualItems, Customizations, Flags, Guild, MapID, etc.)
    result.CopyCharactersToRegionwide();

    // Clear the basic Characters list — we send RegionwideCharacters instead
    result.ClearCharacters();

    // Enrich RegionwideCharacters with money from the database
    uint32 regionCount = result.GetRegionwideCharacterCount();
    for (uint32 i = 0; i < regionCount; i++)
    {
        RegionwideCharacterInfo@ regionChar = result.GetRegionwideCharacter(i);
        if (regionChar is null)
            continue;

        uint64 guid = regionChar.GetGuid();

        // Query money from database
        uint64 money = 0;
        string moneyQuery = "SELECT money FROM characters WHERE guid = " + guid;
        QueryResult@ moneyResult = CharacterQuery(moneyQuery);
        if (moneyResult !is null && moneyResult.GetRowCount() > 0)
        {
            money = moneyResult.GetUInt64(0);
        }
        regionChar.SetMoney(money);

        // TODO: Query average item level
        // regionChar.SetAvgItemLevel(avgIlvlFromDB);
    }

    Print("[CharEnumHook] Done - RegionwideCharacterCount: " + regionCount);
}

void OnPostCharEnum(WorldSession@ session)
{
    // Only send restrictions data in realmless mode
    if (!CONFIG_CHAR_ENUM_REALMLESS)
    {
        Print("[CharEnumHook] OnPostCharEnum - realmless disabled, skipping restrictions");
        return;
    }

    Print("[CharEnumHook] OnPostCharEnum - sending restrictions after char enum");

    uint32 accountId = session.GetAccountId();

    array<uint64> characterGuidsLow;
    array<uint64> characterGuidsHigh;
    array<bool> catchupAvailable;

    // Query all character GUIDs for this account
    string query = "SELECT guid FROM characters WHERE account = " + accountId + " ORDER BY guid";
    QueryResult@ result = CharacterQuery(query);
    if (result !is null && result.GetRowCount() > 0)
    {
        do
        {
            uint64 guid = result.GetUInt64(0);
            uint64 guidLow, guidHigh;
            BuildPlayerGuid(guid, guidLow, guidHigh);
            characterGuidsLow.insertLast(guidLow);
            characterGuidsHigh.insertLast(guidHigh);

            // Check RPE availability: rpe_login DB flag OR eligibility (logout time/first login)
            AngelDBResult rpeResult = AngelDB_Query("SELECT rpe_login FROM character_datas WHERE guid = " + guid);
            bool hasRPE = false;
            if (rpeResult.GetRowCount() > 0 && rpeResult.NextRow())
            {
                uint32 rpeLogin = rpeResult.GetUInt32(0);
                hasRPE = (rpeLogin == 1);
            }

            // If not set in DB, check eligibility based on logout time/first login
            if (!hasRPE)
            {
                hasRPE = IsRPEEligible(guid);
            }

            catchupAvailable.insertLast(hasRPE);
        }
        while (result.NextRow());
    }

    // Send SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA with all character GUIDs and catchup availability
    SendRegionwideCharacterRestrictionsData(session, characterGuidsLow, characterGuidsHigh, catchupAvailable);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuidsLow.length() + " characters");

    // Send SMSG_REGIONWIDE_CHARACTER_MAIL_DATA immediately after restrictions
    SendRegionwideCharacterMailData(session, characterGuidsLow, characterGuidsHigh);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_MAIL_DATA for " + characterGuidsLow.length() + " characters");
}

void OnPostCharDelete(WorldSession@ session)
{
    // Only send restrictions data in realmless mode
    if (!CONFIG_CHAR_ENUM_REALMLESS)
    {
        Print("[CharEnumHook] OnPostCharDelete - realmless disabled, skipping restrictions");
        return;
    }

    Print("[CharEnumHook] OnPostCharDelete - resending restrictions after char delete");

    uint32 accountId = session.GetAccountId();

    array<uint64> characterGuidsLow;
    array<uint64> characterGuidsHigh;
    array<bool> catchupAvailable;

    // Query all character GUIDs for this account (after deletion)
    string query = "SELECT guid FROM characters WHERE account = " + accountId + " ORDER BY guid";
    QueryResult@ result = CharacterQuery(query);
    if (result !is null && result.GetRowCount() > 0)
    {
        do
        {
            uint64 guid = result.GetUInt64(0);
            uint64 guidLow, guidHigh;
            BuildPlayerGuid(guid, guidLow, guidHigh);
            characterGuidsLow.insertLast(guidLow);
            characterGuidsHigh.insertLast(guidHigh);

            // Check RPE availability: rpe_login DB flag OR eligibility (logout time/first login)
            AngelDBResult rpeResult = AngelDB_Query("SELECT rpe_login FROM character_datas WHERE guid = " + guid);
            bool hasRPE = false;
            if (rpeResult.GetRowCount() > 0 && rpeResult.NextRow())
            {
                uint32 rpeLogin = rpeResult.GetUInt32(0);
                hasRPE = (rpeLogin == 1);
            }

            // If not set in DB, check eligibility based on logout time/first login
            if (!hasRPE)
            {
                hasRPE = IsRPEEligible(guid);
            }

            catchupAvailable.insertLast(hasRPE);
        }
        while (result.NextRow());
    }

    // Send SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA with updated character GUIDs and catchup availability
    SendRegionwideCharacterRestrictionsData(session, characterGuidsLow, characterGuidsHigh, catchupAvailable);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuidsLow.length() + " characters after delete");

    // Send SMSG_REGIONWIDE_CHARACTER_MAIL_DATA immediately after restrictions
    SendRegionwideCharacterMailData(session, characterGuidsLow, characterGuidsHigh);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_MAIL_DATA for " + characterGuidsLow.length() + " characters after delete");
}

void main()
{
    RegisterCharEnumHook(@OnCharEnum);
    RegisterPostCharEnumHook(@OnPostCharEnum);
    RegisterPostCharDeleteHook(@OnPostCharDelete);
    Print("[CharEnumHook] Registered OnCharEnum + OnPostCharEnum + OnPostCharDelete hooks for RegionwideCharacter data");
}
