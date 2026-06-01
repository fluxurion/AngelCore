/*
 * RegionwideCharacterPackets.as
 * Packet functions for regionwide character data
 * These are CHARACTER packets, not BattlePay packets
 */
#include "RegionwideCharacterOpcodes.as"

// ============================================================================
// SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA (0x420019)
// Structure: Count + [Flags, Guid, RestrictionID] per character
// Flags: bit 5-7 = TopBits, bit 4 = IsRestricted, bit 3 = CatchUpAvailable
// ============================================================================
void SendRegionwideCharacterRestrictionsData(WorldSession@ session, array<uint64>@ characterGuidsLow, array<uint64>@ characterGuidsHigh)
{
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA);

    uint32 count = characterGuidsLow.length();
    pd.WriteUInt32(count);

    for (uint32 i = 0; i < count; i++)
    {
        uint8 flags = 0;
        // Flags breakdown:
        // bits 5-7 (0xE0) = TopBits (usually 0)
        // bit 4 (0x10) = IsRestricted (false = 0)
        // bit 3 (0x08) = CatchUpAvailable (true = 1, false = 0)
        // For now, set CatchUpAvailable = true (0x08) like most entries in retail dump
        flags = 0x08;  // CatchUpAvailable = true

        pd.WriteUInt8(flags);
        pd.WritePackedGuid(characterGuidsLow[i], characterGuidsHigh[i]);
        pd.WriteUInt32(0);  // RestrictionID = 0 (no restrictions)
    }

    session.SendPacket(pd);
}

// ============================================================================
// SMSG_REGIONWIDE_CHARACTER_MAIL_DATA (0x42001A)
// Structure: Count + [Type(byte), Guid(packed128), SenderCount, Senders[], EntryCount, Entries[Guid, Subject]]
// Type: upper 3 bits of first byte
// For now: sending empty mail data (0 senders, 0 entries) as stub
// ============================================================================
void SendRegionwideCharacterMailData(WorldSession@ session, array<uint64>@ characterGuidsLow, array<uint64>@ characterGuidsHigh)
{
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_MAIL_DATA);

    uint32 count = characterGuidsLow.length();
    pd.WriteUInt32(count);

    for (uint32 i = 0; i < count; i++)
    {
        uint8 type = 0;  // Type in upper 3 bits (0 for now)
        pd.WriteUInt8(type);
        pd.WritePackedGuid(characterGuidsLow[i], characterGuidsHigh[i]);

        // MailSenderCount = 0 (no senders)
        pd.WriteUInt32(0);

        // MailEntryCount = 0 (no mail entries)
        pd.WriteUInt32(0);
    }

    session.SendPacket(pd);
}
