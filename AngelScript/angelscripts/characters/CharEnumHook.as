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
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

void OnCharEnum(WorldSession@ session, EnumCharactersResult@ result)
{
    if (result is null)
    {
        Print("[CharEnumHook] ERROR: result is null!");
        return;
    }

    uint32 charCount = result.GetCharacterCount();
    Print("[CharEnumHook] Called - CharacterCount: " + charCount);

    if (charCount == 0)
        return;

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
    Print("[CharEnumHook] OnPostCharEnum - sending restrictions after char enum");

    uint32 accountId = session.GetAccountId();

    array<uint64> characterGuidsLow;
    array<uint64> characterGuidsHigh;

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
        }
        while (result.NextRow());
    }

    // Send SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA with all character GUIDs
    SendRegionwideCharacterRestrictionsData(session, characterGuidsLow, characterGuidsHigh);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuidsLow.length() + " characters");
}

void main()
{
    RegisterCharEnumHook(@OnCharEnum);
    RegisterPostCharEnumHook(@OnPostCharEnum);
    Print("[CharEnumHook] Registered OnCharEnum + OnPostCharEnum hooks for RegionwideCharacter data");
}
