/*
 * CharEnumHook.as
 * AngelScript hook for SMSG_ENUM_CHARACTERS_RESULT
 * Populates RegionwideCharacterInfo data from database
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

    Print("[CharEnumHook] Called - CharacterCount: " + result.GetCharacterCount());

    // Clear existing RegionwideCharacters (TC doesn't populate them)
    result.ClearRegionwideCharacters();

    // Collect GUIDs for restrictions packet
    array<uint64> characterGuidsLow;
    array<uint64> characterGuidsHigh;

    // Get all regular characters from the result
    uint32 charCount = result.GetCharacterCount();
    for (uint32 i = 0; i < charCount; i++)
    {
        CharEnumCharacterInfo@ charInfo = result.GetCharacter(i);
        if (charInfo is null)
            continue;

        uint64 guid = charInfo.GetGuid();
        // Build full player GUID with correct high part encoding realm and type info
        uint64 guidLow, guidHigh;
        BuildPlayerGuid(guid, guidLow, guidHigh);
        characterGuidsLow.insertLast(guidLow);
        characterGuidsHigh.insertLast(guidHigh);

        // Get character data with explicit initialization
        string name = charInfo.GetName();
        uint8 race = 0;
        uint8 classId = 0;
        uint8 gender = 0;
        uint8 level = 0;

        // Safely get character attributes
        race = charInfo.GetRace();
        classId = charInfo.GetClass();
        gender = charInfo.GetGender();
        level = charInfo.GetLevel();

        // Query money from database
        // CharacterQuery uses TC's CharacterDatabase (the DB configured in worldserver.conf).
        // NOTE: the ResultSet is already positioned at the first row on return, so we read
        // directly. Call NextRow() only to advance to subsequent rows.
        uint64 money = 0;
        string moneyQuery = "SELECT money FROM characters WHERE guid = " + guid;
        QueryResult@ moneyResult = CharacterQuery(moneyQuery);
        if (moneyResult !is null && moneyResult.GetRowCount() > 0)
        {
            money = moneyResult.GetUInt64(0);
        }

        // TODO: Query average item level from world.item_template via C++ API
        // item_instance table doesn't have itemLevel column directly
        float avgIlvl = 0.0f;

        // Add new RegionwideCharacter with all data
        RegionwideCharacterInfo@ regionChar = result.AddRegionwideCharacter(
            guid, name, race, classId, gender, level, money, avgIlvl);

        if (regionChar !is null)
        {
            Print("[CharEnumHook] Added RegionwideChar: " + name + " Money: " + money + " ILvl: " + avgIlvl);
        }
        else
        {
            Print("[CharEnumHook] ERROR: Failed to add RegionwideChar: " + name);
        }

        // TODO: Mythic+ Score and PvP Rating when custom tables are ready
        // if (regionChar !is null)
        // {
        //     regionChar.SetMythicPlusScore(...);
        //     regionChar.SetPvpRating(...);
        // }
    }

    Print("[CharEnumHook] Done - RegionwideCharacterCount: " + result.GetRegionwideCharacterCount());

    // Send SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA with all character GUIDs
    SendRegionwideCharacterRestrictionsData(session, characterGuidsLow, characterGuidsHigh);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuidsLow.length() + " characters");
}

void main()
{
    RegisterCharEnumHook(@OnCharEnum);
    Print("[CharEnumHook] Registered OnCharEnum hook for RegionwideCharacter data");
}
