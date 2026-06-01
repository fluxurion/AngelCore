/*
 * RegionwideCharacterHandlers.as
 * Handlers for regionwide character data (restrictions, mail, etc.)
 * These are CHARACTER handlers, not BattlePay handlers
 */
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

// ============================================================================
// CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA (0x400177)
// Structure: Count + [Guid1(packed128), Guid2(packed128)] per entry
// Response: SMSG_REGIONWIDE_CHARACTER_MAIL_DATA (0x42001A)
// ============================================================================
bool HandleGetRegionwideCharacterRestrictionAndMailData(WorldSession@ session, PacketData@ packet)
{
    // Read the count of characters client is requesting data for
    uint32 count = packet.ReadUInt32();

    // Collect all GUIDs from the request
    array<uint64> characterGuidsLow;
    array<uint64> characterGuidsHigh;
    for (uint32 i = 0; i < count; i++)
    {
        uint64 guid1Low, guid1High, guid2Low, guid2High;
        packet.ReadPackedGuid(guid1Low, guid1High);
        packet.ReadPackedGuid(guid2Low, guid2High);
        // Use guid1 as the character GUID
        characterGuidsLow.insertLast(guid1Low);
        characterGuidsHigh.insertLast(guid1High);
    }

    Print("[RegionwideCharacter] CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA for " + count + " characters");

    // Send SMSG_REGIONWIDE_CHARACTER_MAIL_DATA response
    // Note: Restrictions are sent via SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
    SendRegionwideCharacterMailData(session, characterGuidsLow, characterGuidsHigh);
    return true;
}

void main()
{
    // Register regionwide character data handlers
    RegisterOpcodeHandler(CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA, @HandleGetRegionwideCharacterRestrictionAndMailData, false);

    Print("[RegionwideCharacter] Initialized handlers");
}
