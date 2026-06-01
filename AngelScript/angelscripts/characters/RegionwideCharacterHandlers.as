/*
 * RegionwideCharacterHandlers.as
 * Handlers for regionwide character data (restrictions, mail, etc.)
 * These are CHARACTER handlers, not BattlePay handlers
 */
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

// ============================================================================
// CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA (0x400177)
// Structure: Single byte flag/value
// Response: SMSG_REGIONWIDE_CHARACTER_MAIL_DATA (0x42001A)
// Note: Client sends this after receiving SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
// ============================================================================
bool HandleGetRegionwideCharacterRestrictionAndMailData(WorldSession@ session, PacketData@ packet)
{
    // Read single byte (purpose unknown, possibly flags or request type)
    uint8 unknownByte = packet.ReadUInt8();

    Print("[RegionwideCharacter] CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA byte: " + unknownByte);

    // Send empty SMSG_REGIONWIDE_CHARACTER_MAIL_DATA response
    // TODO: When proper character storage is available, send data for all account characters
    array<uint64> emptyGuidsLow;
    array<uint64> emptyGuidsHigh;
    SendRegionwideCharacterMailData(session, emptyGuidsLow, emptyGuidsHigh);
    return true;
}

void main()
{
    // Register regionwide character data handlers
    RegisterOpcodeHandler(CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA, @HandleGetRegionwideCharacterRestrictionAndMailData, false);

    Print("[RegionwideCharacter] Initialized handlers");
}
