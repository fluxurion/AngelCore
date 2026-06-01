/*
 * CharEnumHook.as
 * AngelScript hook for SMSG_ENUM_CHARACTERS_RESULT
 * Populates RegionwideCharacterInfo data from database
 */

void OnCharEnum(WorldSession@ session, EnumCharactersResult@ result)
{
    if (result is null)
        return;

    uint32 count = result.GetRegionwideCharacterCount();
    for (uint32 i = 0; i < count; i++)
    {
        RegionwideCharacterInfo@ charInfo = result.GetRegionwideCharacter(i);
        if (charInfo is null)
            continue;

        uint64 guid = charInfo.GetGuid();

        // 1. Money (characters.money)
        QueryResult@ moneyResult = CharacterQuery(
            "SELECT money FROM characters WHERE guid = " + guid);
        if (moneyResult !is null && moneyResult.NextRow())
        {
            charInfo.SetMoney(moneyResult.GetUInt64(0));
        }

        // 2. Average Item Level (character_inventory + item_instance)
        QueryResult@ ilvlResult = CharacterQuery(
            "SELECT AVG(ii.itemLevel) FROM character_inventory ci " +
            "JOIN item_instance ii ON ci.item = ii.guid " +
            "WHERE ci.guid = " + guid + " AND ci.bag = 0 AND ci.slot < 19");
        if (ilvlResult !is null && ilvlResult.NextRow())
        {
            float avgIlvl = ilvlResult.GetFloat(0);
            if (avgIlvl > 0)
                charInfo.SetAvgItemLevel(avgIlvl);
        }

        // 3. Mythic+ Score - TODO: implement custom table
        // QueryResult@ mythicResult = CharacterQuery(
        //     "SELECT overall_score FROM character_mythic_plus WHERE guid = " + guid);
        // if (mythicResult !is null && mythicResult.NextRow())
        //     charInfo.SetMythicPlusScore(mythicResult.GetFloat(0));

        // 4. PvP Rating - TODO: implement custom table
        // QueryResult@ pvpResult = CharacterQuery(
        //     "SELECT rating, bracket, spec_id FROM character_pvp_rating " +
        //     "WHERE guid = " + guid + " ORDER BY season DESC LIMIT 1");
        // if (pvpResult !is null && pvpResult.NextRow())
        // {
        //     charInfo.SetPvpRating(pvpResult.GetUInt32(0));
        //     charInfo.SetPvpBracket(pvpResult.GetInt8(1));
        //     charInfo.SetPvpSpecId(pvpResult.GetInt16(2));
        // }
    }
}

void main()
{
    RegisterCharEnumHook(@OnCharEnum);
    Print("[CharEnumHook] Registered OnCharEnum hook for RegionwideCharacter data");
}
