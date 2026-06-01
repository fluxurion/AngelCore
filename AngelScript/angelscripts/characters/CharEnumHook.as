/*
 * CharEnumHook.as
 * AngelScript hooks for SMSG_ENUM_CHARACTERS_RESULT
 *
 * OnCharEnum (PRE-send):  Populates RegionwideCharacterInfo from TC's
 *                          Character entries (money, ilvl, etc.).
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

    // Collect character data from TC's Characters list
    array<string>  names;
    array<uint64>  guids;
    array<uint8>   races;
    array<uint8>   classes;
    array<uint8>   genders;
    array<uint8>   levels;

    for (uint32 i = 0; i < charCount; i++)
    {
        CharEnumCharacterInfo@ charInfo = result.GetCharacter(i);
        if (charInfo is null)
            continue;

        guids.insertLast(charInfo.GetGuid());
        names.insertLast(charInfo.GetName());
        races.insertLast(charInfo.GetRace());
        classes.insertLast(charInfo.GetClass());
        genders.insertLast(charInfo.GetGender());
        levels.insertLast(charInfo.GetLevel());
    }

    // Clear existing RegionwideCharacters (TC doesn't populate them)
    result.ClearRegionwideCharacters();

    // Populate RegionwideCharacters from the collected data
    for (uint32 i = 0; i < guids.length(); i++)
    {
        uint64 guid = guids[i];

        // Query money from database
        uint64 money = 0;
        string moneyQuery = "SELECT money FROM characters WHERE guid = " + guid;
        QueryResult@ moneyResult = CharacterQuery(moneyQuery);
        if (moneyResult !is null && moneyResult.GetRowCount() > 0)
        {
            money = moneyResult.GetUInt64(0);
        }

        // TODO: Query average item level from world.item_template via C++ API
        float avgIlvl = 0.0f;

        // Add new RegionwideCharacter with all data
        RegionwideCharacterInfo@ regionChar = result.AddRegionwideCharacter(
            guid, names[i], races[i], classes[i], genders[i], levels[i], money, avgIlvl);

        if (regionChar !is null)
        {
            Print("[CharEnumHook] Added RegionwideChar: " + names[i] + " Money: " + money + " ILvl: " + avgIlvl);
        }
        else
        {
            Print("[CharEnumHook] ERROR: Failed to add RegionwideChar: " + names[i]);
        }
    }

    Print("[CharEnumHook] Done - RegionwideCharacterCount: " + result.GetRegionwideCharacterCount());
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
