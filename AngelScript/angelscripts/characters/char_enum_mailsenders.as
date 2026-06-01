// ============================================================
// Character Enum Mail Senders
// NOTE: This handler is DISABLED - mail data is now sent proactively
// immediately after SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
// in CharEnumHook.as instead of waiting for CMSG request.
// ============================================================

#include "../includes/ScriptFramework.as"
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

// ============================================================
// Main entry point
// ============================================================
void main()
{
    // Handler disabled - mail data sent proactively in CharEnumHook.as
    // RegisterOpcodeHandler(CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA, @HandleRegionwideMailData, false);
    Print("[CharEnum] Regionwide mail data handler DISABLED - sent proactively in CharEnumHook.as");
}
