/*
 * CharEnumHook.as
 * AngelScript hooks for SMSG_ENUM_CHARACTERS_RESULT
 *
 * OnCharEnum (PRE-send):  Copies all CharacterInfoBasic data from TC's
 *                          Characters into RegionwideCharacters, clears
 *                          the Characters list, then enriches with money,
 *                          avg item level, and profession IDs.
 * OnPostCharEnum (POST-send): Sends SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
 *                             after SMSG_ENUM_CHARACTERS_RESULT.
 */
#include "../Config.as"
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

// Profession SkillLine IDs (retail 11.x) — matched against character_skills
// to populate ProfessionIds[2] on RegionwideCharacter entries.
const uint32 PROFESSION_SKILL_IDS[] = {
    202, 773, 333, 755, 164, 171, 165, 197, 186, 182, 393
};

// ============================================================================
// Check if character is eligible for RPE/catchup
// ============================================================================
bool IsRPEEligible(uint64 characterGuid)
{
    string query = "SELECT logout_time FROM characters WHERE guid = " + characterGuid;
    QueryResult@ result = CharacterQuery(query);

    if (result is null || result.GetRowCount() == 0)
        return false;

    uint64 logoutTime = result.GetUInt64(0);

    if (logoutTime == 0)
    {
        Print("[CharEnumHook] Character " + characterGuid + " has no logout_time (new character)");
        return CONFIG_RPE_ALLOW_FIRST_LOGIN;
    }

    uint64 currentTime = GetUnixTime();
    uint64 secondsSinceLogout = currentTime - logoutTime;
    uint64 daysSinceLogout = secondsSinceLogout / 86400;

    Print("[CharEnumHook] Character " + characterGuid + " logged out " + daysSinceLogout + " days ago");

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

    result.SetRealmless(CONFIG_CHAR_ENUM_REALMLESS);

    if (charCount == 0)
        return;

    if (!CONFIG_CHAR_ENUM_REALMLESS)
    {
        Print("[CharEnumHook] Realmless disabled - using basic character data");
        return;
    }

    result.CopyCharactersToRegionwide();
    result.ClearCharacters();

    // Enrich RegionwideCharacters with money, avg item level, and profession IDs
    uint32 regionCount = result.GetRegionwideCharacterCount();
    for (uint32 i = 0; i < regionCount; i++)
    {
        RegionwideCharacterInfo@ regionChar = result.GetRegionwideCharacter(i);
        if (regionChar is null)
            continue;

        uint64 guid = regionChar.GetGuid();

        // Query money
        uint64 money = 0;
        string moneyQuery = "SELECT money FROM characters WHERE guid = " + guid;
        QueryResult@ moneyResult = CharacterQuery(moneyQuery);
        if (moneyResult !is null && moneyResult.GetRowCount() > 0)
        {
            money = moneyResult.GetUInt64(0);
        }
        regionChar.SetMoney(money);

        // Query avg item level from character_datas (saved by SaveAvgItemLevel.as on logout)
        AngelDBResult avgResult = AngelDB_Query(
            "SELECT avgitemlevel FROM character_datas WHERE guid = " + guid
        );
        if (avgResult.GetRowCount() > 0 && avgResult.NextRow())
        {
            float avgILvl = avgResult.GetFloat(0);
            regionChar.SetAvgItemLevel(avgILvl);
        }

        // Query profession IDs from character_skills
        string skillsQuery = "SELECT skill FROM character_skills WHERE guid = " + guid;
        QueryResult@ skillsResult = CharacterQuery(skillsQuery);
        if (skillsResult !is null && skillsResult.GetRowCount() > 0)
        {
            uint32 profSlot = 0;
            do
            {
                uint32 skillId = skillsResult.GetUInt32(0);
                for (uint32 pi = 0; pi < PROFESSION_SKILL_IDS.length() && profSlot < 2; pi++)
                {
                    if (skillId == PROFESSION_SKILL_IDS[pi])
                    {
                        regionChar.SetProfessionId(profSlot, int32(skillId));
                        profSlot++;
                        break;
                    }
                }
            }
            while (skillsResult.NextRow() && profSlot < 2);
        }
    }

    Print("[CharEnumHook] Done - RegionwideCharacterCount: " + regionCount);
}

void OnPostCharEnum(WorldSession@ session)
{
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

            AngelDBResult rpeResult = AngelDB_Query("SELECT rpe_login FROM character_datas WHERE guid = " + guid);
            bool hasRPE = false;
            if (rpeResult.GetRowCount() > 0 && rpeResult.NextRow())
            {
                uint32 rpeLogin = rpeResult.GetUInt32(0);
                hasRPE = (rpeLogin == 1);
            }

            if (!hasRPE)
            {
                hasRPE = IsRPEEligible(guid);
            }

            catchupAvailable.insertLast(hasRPE);
        }
        while (result.NextRow());
    }

    SendRegionwideCharacterRestrictionsData(session, characterGuidsLow, characterGuidsHigh, catchupAvailable);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuidsLow.length() + " characters");

    SendRegionwideCharacterMailData(session, characterGuidsLow, characterGuidsHigh);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_MAIL_DATA for " + characterGuidsLow.length() + " characters");
}

void OnPostCharDelete(WorldSession@ session)
{
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

            AngelDBResult rpeResult = AngelDB_Query("SELECT rpe_login FROM character_datas WHERE guid = " + guid);
            bool hasRPE = false;
            if (rpeResult.GetRowCount() > 0 && rpeResult.NextRow())
            {
                uint32 rpeLogin = rpeResult.GetUInt32(0);
                hasRPE = (rpeLogin == 1);
            }

            if (!hasRPE)
            {
                hasRPE = IsRPEEligible(guid);
            }

            catchupAvailable.insertLast(hasRPE);
        }
        while (result.NextRow());
    }

    SendRegionwideCharacterRestrictionsData(session, characterGuidsLow, characterGuidsHigh, catchupAvailable);
    Print("[CharEnumHook] Sent SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA for " + characterGuidsLow.length() + " characters after delete");

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
