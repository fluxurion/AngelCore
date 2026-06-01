/*
 * CharEnumHook.as
 * AngelScript hook for SMSG_ENUM_CHARACTERS_RESULT
 * Populates RegionwideCharacterInfo data from database
 */

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
    array<ObjectGuid> characterGuids;

    // Get all regular characters from the result
    uint32 charCount = result.GetCharacterCount();
    for (uint32 i = 0; i < charCount; i++)
    {
        CharEnumCharacterInfo@ charInfo = result.GetCharacter(i);
        if (charInfo is null)
            continue;

        uint64 guid = charInfo.GetGuid();
        ObjectGuid objectGuid = ObjectGuid(guid);
        characterGuids.insertLast(objectGuid);

        string name = charInfo.GetName();
        uint8 race = charInfo.GetRace();
        uint8 classId = charInfo.GetClass();
        uint8 gender = charInfo.GetGender();
        uint8 level = charInfo.GetLevel();

        // Query money from database
        uint64 money = 0;
        QueryResult@ moneyResult = CharacterQuery(
            "SELECT money FROM characters WHERE guid = " + guid);
        if (moneyResult !is null && moneyResult.NextRow())
        {
            money = moneyResult.GetUInt64(0);
        }

        // Query average item level from database
        float avgIlvl = 0.0f;
        QueryResult@ ilvlResult = CharacterQuery(
            "SELECT AVG(ii.itemLevel) FROM character_inventory ci " +
            "JOIN item_instance ii ON ci.item = ii.guid " +
            "WHERE ci.guid = " + guid + " AND ci.bag = 0 AND ci.slot < 19");
        if (ilvlResult !is null && ilvlResult.NextRow())
        {
            avgIlvl = ilvlResult.GetFloat(0);
        }

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
    SendRegionwideCharacterRestrictionsData(session, characterGuids);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuids.length() + " characters");
}

void main()
{
    RegisterCharEnumHook(@OnCharEnum);
    Print("[CharEnumHook] Registered OnCharEnum hook for RegionwideCharacter data");
}
