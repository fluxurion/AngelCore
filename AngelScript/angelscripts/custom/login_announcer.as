/*
 * Login Announcer
 * Announces player logins with class, race, gender, faction and level info
 */

#include "includes/ScriptFramework.as"
#include "includes/Common.as"

string GetClassIcon(uint8 classId)
{
    switch (classId)
    {
        case CLASS_WARLOCK:        return "|TInterface\\icons\\classicon_warlock:15|t";
        case CLASS_WARRIOR:        return "|TInterface\\icons\\classicon_warrior:15|t";
        case CLASS_MAGE:           return "|TInterface\\icons\\classicon_mage:15|t";
        case CLASS_SHAMAN:         return "|TInterface\\icons\\classicon_shaman:15|t";
        case CLASS_DEATH_KNIGHT:   return "|TInterface\\icons\\spell_deathknight_classicon:15|t";
        case CLASS_DRUID:          return "|TInterface\\icons\\classicon_druid:15|t";
        case CLASS_HUNTER:         return "|TInterface\\icons\\classicon_hunter:15|t";
        case CLASS_PALADIN:        return "|TInterface\\icons\\classicon_paladin:15|t";
        case CLASS_ROGUE:          return "|TInterface\\icons\\classicon_rogue:15|t";
        case CLASS_PRIEST:         return "|TInterface\\icons\\classicon_priest:15|t";
        case CLASS_DEMON_HUNTER:   return "|TInterface\\icons\\classicon_demonhunter:15|t";
        case CLASS_MONK:           return "|TInterface\\icons\\classicon_monk:15|t";
        case CLASS_EVOKER:         return "|TInterface\\icons\\classicon_evoker:15|t";
        case CLASS_ADVENTURER:     return "|TInterface\\icons\\classicon_evoker:15|t";
        case CLASS_TRAVELER:       return "|TInterface\\icons\\classicon_evoker:15|t";
    }
    return "";
}

string GetGenderString(uint8 gender)
{
    switch (gender)
    {
        case GENDER_MALE:    return "Male";
        case GENDER_FEMALE:  return "Female";
        case GENDER_NONE:    return "None";
    }
    return "Unknown";
}

string GetRaceIcon(uint8 race, uint8 gender)
{
    bool isMale = (gender == GENDER_MALE);
    
    switch (race)
    {
        case RACE_HUMAN:
            return isMale ? "|TInterface\\icons\\achievement_character_human_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_human_female:15|t";
        case RACE_ORC:
            return isMale ? "|TInterface\\icons\\achievement_character_orc_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_orc_female:15|t";
        case RACE_DWARF:
            return isMale ? "|TInterface\\icons\\achievement_character_dwarf_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_dwarf_female:15|t";
        case RACE_NIGHTELF:
            return isMale ? "|TInterface\\icons\\achievement_character_nightelf_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_nightelf_female:15|t";
        case RACE_UNDEAD:
            return isMale ? "|TInterface\\icons\\achievement_character_undead_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_undead_female:15|t";
        case RACE_TAUREN:
            return isMale ? "|TInterface\\icons\\achievement_character_tauren_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_tauren_female:15|t";
        case RACE_GNOME:
            return isMale ? "|TInterface\\icons\\achievement_character_gnome_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_gnome_female:15|t";
        case RACE_TROLL:
            return isMale ? "|TInterface\\icons\\achievement_character_troll_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_troll_female:15|t";
        case RACE_GOBLIN:
            return isMale ? "|TInterface\\icons\\achievement_goblinhead:15|t" 
                          : "|TInterface\\icons\\achievement_femalegoblinhead:15|t";
        case RACE_BLOODELF:
            return isMale ? "|TInterface\\icons\\achievement_character_bloodelf_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_bloodelf_female:15|t";
        case RACE_DRAENEI:
            return isMale ? "|TInterface\\icons\\achievement_character_draenei_male:15|t" 
                          : "|TInterface\\icons\\achievement_character_draenei_female:15|t";
        case RACE_WORGEN:
            return "|TInterface\\icons\\achievement_worganhead:15|t";
        case RACE_PANDAREN_NEUTRAL:
        case RACE_PANDAREN_HORDE:
        case RACE_PANDAREN_ALLIANCE:
            return isMale ? "|TInterface\\icons\\pandarenracial_innerpeace:15|t" 
                          : "|TInterface\\icons\\achievement_character_pandaren_female:15|t";
        case RACE_NIGHTBORNE:
            return isMale ? "|TInterface\\icons\\inv_nightbornemale:15|t" 
                          : "|TInterface\\icons\\inv_nightbornefemale:15|t";
        case RACE_VOID_ELF:
            return "|TInterface\\icons\\achievement_alliedrace_voidelf:15|t";
        case RACE_HIGHMOUNTAIN_TAUREN:
            return "|TInterface\\icons\\achievement_alliedrace_highmountaintauren:15|t";
        case RACE_LIGHTFORGED_DRAENEI:
            return "|TInterface\\icons\\achievement_alliedrace_lightforgeddraenei:15|t";
        case RACE_ZANDALARI_TROLL:
            return isMale ? "|TInterface\\icons\\inv_zandalarimalehead:15|t" 
                          : "|TInterface\\icons\\inv_zandalarifemalehead:15|t";
        case RACE_KUL_TIRAN:
            return "|TInterface\\icons\\achievement_alliedrace_kultiranhuman:15|t";
        case RACE_DARK_IRON_DWARF:
            return "|TInterface\\icons\\achievement_alliedrace_darkirondwarf:15|t";
        case RACE_VULPERA:
            return "|TInterface\\icons\\achievement_alliedrace_vulpera:15|t";
        case RACE_MAGHAR_ORC:
            return isMale ? "|TInterface\\icons\\achievement_character_orc_male_brn:15|t" 
                          : "|TInterface\\icons\\achievement_character_orc_female_brn:15|t";
        case RACE_MECHAGNOME:
            return "|TInterface\\icons\\achievement_alliedrace_mechagnome:15|t";
        case RACE_DRACTHYR_ALLIANCE:
        case RACE_DRACTHYR_HORDE:
            return "|TInterface\\icons\\ui_dracthyr:15|t";
        case RACE_EARTHEN_DWARF_ALLIANCE:
        case RACE_EARTHEN_DWARF_HORDE:
            return "|TInterface\\icons\\inv_achievement_alliedrace_earthen:15|t";
        case RACE_HARANIR_ALLIANCE:
        case RACE_HARANIR_HORDE:
            return "|TInterface\\icons\\inv12_achievements_alliedrace_haranir_sigil:15|t";
    }
    return "";
}

string GetFactionIcon(int team)
{
    switch (team)
    {
        case TEAM_ALLIANCE: return "|cff0026FF|TInterface\\icons\\ui_allianceicon:15|t|r";
        case TEAM_HORDE:    return "|cffFF0000|TInterface\\icons\\ui_hordeicon:15|t|r";
        case TEAM_NEUTRAL:  return "|cFFF5F5DC|TInterface\\icons\\vas_guildfactionchange:15|t|r";
    }
    return "";
}

string GetRandomLoginText()
{
    int rand = int(GetGameTime() % 4);
    switch (rand)
    {
        case 0: return "|cff4CFF00 has entered the world!|r";
        case 1: return "|cff4CFF00 has logged in!|r";
        case 2: return "|cff4CFF00 has spawned!|r";
        case 3: return "|cff4CFF00 has dropped in!|r";
    }
    return "|cff4CFF00 has entered the world!|r";
}

void OnLogin(Player@ player)
{
    if (player is null)
        return;

    uint32 pLevel = player.GetLevel();
    uint8 pClass = player.GetClass();
    uint8 pRace = player.GetRace();
    uint8 pGender = player.GetGender();
    int team = GetTeam(player);

    string pClassIcon = GetClassIcon(pClass);
    string pRaceIcon = GetRaceIcon(pRace, pGender);
    string pFactionIcon = GetFactionIcon(team);
    string pLoginText = GetRandomLoginText();

    string loginmsg = "|cff3DAEFF|TInterface\\icons\\inv_misc_token_boost_generic:15|t [Login Announcer]: ";
    loginmsg += "|cffFFD800|TInterface\\icons\\inv_prg_icon_puzzle04:15|t";
    loginmsg += player.GetName();
    loginmsg += pLoginText;
    loginmsg += " |TInterface\\icons\\inv_prg_icon_puzzle02:15|t";
    loginmsg += "|cff3DAEFF [";
    loginmsg += pLevel;
    loginmsg += "] ";
    loginmsg += pFactionIcon;
    loginmsg += " ";
    loginmsg += pRaceIcon;
    loginmsg += " ";
    loginmsg += pClassIcon;
    loginmsg += "|r";

    // Log to console
    Print("[LoginAnnouncer]: " + player.GetName() + " (Level " + pLevel + ") has entered the world!");
    
    // Send to all online players via server message
    SendWorldText(loginmsg);
}

void main()
{
    Print("Login Announcer loaded!");
    RegisterPlayerScript(PLAYER_ON_LOGIN, @OnLogin);
}
