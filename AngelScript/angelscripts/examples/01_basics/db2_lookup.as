/*
 * Example 01: DB2 Data Lookup
 * Look up spell names, item names, map names from loaded DB2 stores.
 */

#include "../includes/ScriptFramework.as"

void OnStartup()
{
    // Look up data from DB2 stores
    string spell = GetSpellName(133);
    string item  = GetItemName(17349);
    string map   = GetMapName(0);
    string area  = GetAreaName(1519);
    string cls   = GetClassName(1);
    string race  = GetRaceName(1);

    Print("Fireball spell: " + spell);
    Print("Item 17349: " + item);
    Print("Map 0: " + map);
    Print("Area 1519: " + area);
    Print("Class 1: " + cls);
    Print("Race 1: " + race);

    // Check if something exists
    if (HasSpellDB(133))
        Print("Spell 133 exists in DB2");
    else
        Print("Spell 133 NOT found");

    // Count entries in a store
    uint32 spellCount = GetDB2StoreEntryCount(DB2_STORE_SPELL_NAME);
    Print("Total spells in DB2: " + spellCount);
}

void main()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnStartup);
}
