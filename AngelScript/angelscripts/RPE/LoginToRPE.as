/*
 * LoginToRPE.as
 * Handles RPE (Recruit a Friend Experience) login
 * Checks logout_time to determine RPE/catchup eligibility
 */

#include "../includes/ScriptFramework.as"
#include "../Config.as"

// ============================================================================
// RPE Start Position
// ============================================================================
const uint32 RPE_MAP_ID = 2927;
const float RPE_POS_X = -1101.67;
const float RPE_POS_Y = -3554.37;
const float RPE_POS_Z = 48.9203;
const float RPE_ORIENTATION = 6.258366584777832031;

// ============================================================================
// RPE Spell IDs
// ============================================================================
const uint32 SPELL_APPLY_RPE = 1236914;           // Apply RPE (DNT)
const uint32 SPELL_PLAY_TIMELINE_SCENE = 1237116; // Play Timeline Scene (DNT)
const uint32 SPELL_APPLY_RPE_WALKING = 1236321;   // Apply RPE Walking (DNT) - on scene end
const uint32 SPELL_PLAY_CAMERA_GRAB_SCENE = 1248494; // Play Camera Grab Scene (DNT) - on scene end

// ============================================================================
// Check if character is eligible for RPE/catchup
// ============================================================================
bool IsRPEEligible(uint64 characterGuid)
{
    // Query logout_time from characters table
    string query = "SELECT logout_time, login_time FROM characters WHERE guid = " + characterGuid;
    QueryResult@ result = CharacterQuery(query);

    if (result is null || result.GetRowCount() == 0)
        return false;

    uint64 logoutTime = result.GetUInt64(0);
    uint64 loginTime = result.GetUInt64(1);

    // Check for new character (never logged in or logout_time = 0)
    if (logoutTime == 0)
    {
        Print("[RPE] Character " + characterGuid + " has no logout_time (new character)");
        return CONFIG_RPE_ALLOW_FIRST_LOGIN;
    }

    // Calculate days since logout
    // logout_time is Unix timestamp in seconds
    uint64 currentTime = GetUnixTime();
    uint64 secondsSinceLogout = currentTime - logoutTime;
    uint64 daysSinceLogout = secondsSinceLogout / 86400;  // 86400 seconds in a day

    Print("[RPE] Character " + characterGuid + " logged out " + daysSinceLogout + " days ago");

    // Check if enough time has passed
    if (daysSinceLogout >= CONFIG_RPE_REQUIRED_LOGOUT_DAYS)
    {
        Print("[RPE] Character " + characterGuid + " is eligible for RPE (logout " + daysSinceLogout + " days ago, required: " + CONFIG_RPE_REQUIRED_LOGOUT_DAYS + ")");
        return true;
    }

    Print("[RPE] Character " + characterGuid + " is NOT eligible for RPE (logout " + daysSinceLogout + " days ago, required: " + CONFIG_RPE_REQUIRED_LOGOUT_DAYS + ")");
    return false;
}

// ============================================================================
// Handle CMSG_PLAYER_LOGIN to check RPE flag and set position
// ============================================================================
bool HandlePlayerLogin(WorldSession@ session, PacketData@ packet)
{
    // Read packet data
    uint64 guidLow = packet.ReadUInt64();
    uint64 guidHigh = packet.ReadUInt64();
    float farClip = packet.ReadFloat();
    bool rpe = packet.ReadUInt8() != 0;  // RPE is a bool (1 byte)

    Print("[RPE] PlayerLogin - Guid: " + guidLow + ", FarClip: " + farClip + ", RPE: " + rpe);

    if (rpe)
    {
        // Check eligibility based on logout_time
        bool eligible = IsRPEEligible(guidLow);

        if (eligible)
        {
            Print("[RPE] RPE enabled and eligible - setting position to RPE start zone");
            // Set position in characters table - player will load at this location
            string query = "UPDATE characters SET map = " + RPE_MAP_ID + ", position_x = " + RPE_POS_X + ", position_y = " + RPE_POS_Y + ", position_z = " + RPE_POS_Z + ", orientation = " + RPE_ORIENTATION + " WHERE guid = " + guidLow;
            CharacterQuery(query);

            // Store RPE flag in AngelDB for spell casting after login
            AngelDB_Execute("INSERT INTO character_datas (guid, rpe_login) VALUES (" + guidLow + ", 1) ON DUPLICATE KEY UPDATE rpe_login = 1");
        }
        else
        {
            Print("[RPE] RPE enabled but NOT eligible - skipping RPE");
        }
    }

    // Return false to let TC handle the packet normally
    return false;
}

// ============================================================================
// Handle PLAYER_ON_LOGIN to cast RPE spells
// ============================================================================
void OnPlayerLogin(Player@ player)
{
    if (player is null)
        return;

    uint64 guid = player.GetGUIDLow();

    // Check if this character has RPE login flag in AngelDB
    QueryResult@ result = AngelDB_Query("SELECT rpe_login FROM character_datas WHERE guid = " + guid);

    if (result !is null && result.GetRowCount() > 0)
    {
        uint32 rpeLogin = result.GetUInt32(0);
        if (rpeLogin == 1)
        {
            Print("[RPE] Player " + player.GetName() + " logged in with RPE - casting spells");

            // Cast RPE spells
            player.CastSpell(player, SPELL_APPLY_RPE, true);
            player.CastSpell(player, SPELL_PLAY_TIMELINE_SCENE, true);

            // TODO: Add gear from loadout (commented out for now)
            // AddRPESetGear(player);

            // Clear the flag
            AngelDB_Execute("UPDATE character_datas SET rpe_login = 0 WHERE guid = " + guid);
        }
    }
}

// ============================================================================
// Handle PLAYER_ON_LOGOUT to refresh catchup availability
// This is called when player logs out - logout_time will be updated by TC
// ============================================================================
void OnPlayerLogout(Player@ player)
{
    if (player is null)
        return;

    uint64 guid = player.GetGUIDLow();
    Print("[RPE] Player " + player.GetName() + " logging out - catchup availability will refresh on next login");

    // No action needed - TC updates logout_time automatically
    // The next login will check the new logout_time
}

// ============================================================================
// Handle scene end for RPE walking and camera grab
// ============================================================================
void OnSceneEnd(Player@ player, uint32 sceneId)
{
    if (player is null)
        return;

    // Check if this is the RPE timeline scene
    if (sceneId == SPELL_PLAY_TIMELINE_SCENE)
    {
        Print("[RPE] Scene ended for player " + player.GetName() + " - casting post-scene spells");

        // Cast post-scene spells
        player.CastSpell(player, SPELL_APPLY_RPE_WALKING, true);
        player.CastSpell(player, SPELL_PLAY_CAMERA_GRAB_SCENE, true);
    }
}

// ============================================================================
// Handle scene cancel for RPE
// ============================================================================
void OnSceneCancel(Player@ player, uint32 sceneId)
{
    if (player is null)
        return;

    // Check if this is the RPE timeline scene
    if (sceneId == SPELL_PLAY_TIMELINE_SCENE)
    {
        Print("[RPE] Scene cancelled for player " + player.GetName() + " - handling cleanup");

        // TODO: Add cleanup logic if needed when scene is cancelled
    }
}

// ============================================================================
// Main entry point
// ============================================================================
void main()
{
    RegisterOpcodeHandler(0x400016, @HandlePlayerLogin, false);  // CMSG_PLAYER_LOGIN
    RegisterPlayerScript(PLAYER_ON_LOGIN, @OnPlayerLogin);
    RegisterPlayerScript(PLAYER_ON_LOGOUT, @OnPlayerLogout);
    RegisterPlayerSceneEndHook(@OnSceneEnd);
    RegisterPlayerSceneCancelHook(@OnSceneCancel);
    Print("[RPE] LoginToRPE script loaded");
}
