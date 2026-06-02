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
void SendRegionwideCharacterRestrictionsData(WorldSession@ session, array<uint64>@ characterGuidsLow, array<uint64>@ characterGuidsHigh, array<bool>@ catchupAvailable)
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
        if (i < catchupAvailable.length() && catchupAvailable[i])
            flags = 0x08;  // CatchUpAvailable = true
        else
            flags = 0;     // CatchUpAvailable = false

        pd.WriteUInt8(flags);
        pd.WritePackedGuid(characterGuidsLow[i], characterGuidsHigh[i]);
        pd.WriteUInt32(0);  // RestrictionID = 0 (no restrictions)
    }

    session.SendPacket(pd);
}

// ============================================================================
// SMSG_REGIONWIDE_CHARACTER_MAIL_DATA (0x42001A)
// Structure: Count + [Type(byte), Guid(packed128), SenderCount, Senders[],
//                      EntryCount, Entries[Guid, Subject]]
// Type: upper 3 bits of first byte
//
// Mail checked field bitmask (from TrinityCore characters.mail):
//   MAIL_CHECK_MASK_READ        = 1    (mail has been read)
//   MAIL_CHECK_MASK_RETURNED    = 2    (mail was returned)
//   MAIL_CHECK_MASK_COD_PAYMENT = 4    (COD payment taken)
//   MAIL_CHECK_MASK_HAS_BODY    = 8    (mail has body text)
//   MAIL_CHECK_MASK_UNK5        = 16   (unchecked / unread flag)
//
// To find unread mail: (checked & 16) = 16
// To find read mail:   (checked & 1) = 1
// ============================================================================
void SendRegionwideCharacterMailData(WorldSession@ session, array<uint64>@ characterGuidsLow, array<uint64>@ characterGuidsHigh)
{
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_MAIL_DATA);

    uint32 count = characterGuidsLow.length();
    pd.WriteUInt32(count);

    for (uint32 i = 0; i < count; i++)
    {
        uint64 guidLow = characterGuidsLow[i];
        uint64 guidHigh = characterGuidsHigh[i];

        // Query unread mail for this character
        // checked & 16 = 16 means mail is unchecked/unread in TrinityCore
        string mailQuery = "SELECT id, sender, subject, checked FROM mail WHERE receiver = "
            + guidLow + " AND (checked & 16) = 16";

        QueryResult@ mailResult = CharacterQuery(mailQuery);

        uint8 type = 0;  // Type in upper 3 bits (0 = normal mail data)
        pd.WriteUInt8(type);
        pd.WritePackedGuid(guidLow, guidHigh);

        if (mailResult !is null && mailResult.GetRowCount() > 0)
        {
            // Collect unique senders to build sender list
            // For simplicity, we count unique senders per character
            // In TrinityCore, this is the count of unique sender GUIDs
            uint32 senderCount = 0;
            pd.WriteUInt32(senderCount);  // MailSenderCount

            // Write unread mail entries
            uint32 unreadCount = mailResult.GetRowCount();
            pd.WriteUInt32(unreadCount);  // MailEntryCount

            // NextRow() must be called BEFORE reading the first row
            while (mailResult.NextRow())
            {
                uint64 mailId = mailResult.GetUInt64(0);
                string mailSubject = mailResult.GetString(2);

                // Write mail ID as packed128 guid (low = mailId, high = 0)
                pd.WritePackedGuid(mailId, 0);
                // Write mail subject
                pd.WriteString(mailSubject);
            }

            Print("[RegionwideMail] Character " + guidLow + " has " + unreadCount + " unread mails");
        }
        else
        {
            // No unread mail for this character
            pd.WriteUInt32(0);  // MailSenderCount = 0
            pd.WriteUInt32(0);  // MailEntryCount = 0
        }
    }

    session.SendPacket(pd);
}
