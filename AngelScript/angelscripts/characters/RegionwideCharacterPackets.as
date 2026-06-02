/*
 * RegionwideCharacterPackets.as
 */
#include "RegionwideCharacterOpcodes.as"

void SendRegionwideCharacterRestrictionsData(WorldSession@ session, array<uint64>@ characterGuidsLow, array<uint64>@ characterGuidsHigh, array<bool>@ catchupAvailable)
{
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA);
    uint32 count = characterGuidsLow.length();
    pd.WriteUInt32(count);
    for (uint32 i = 0; i < count; i++)
    {
        uint8 flags = 0;
        if (i < catchupAvailable.length() && catchupAvailable[i])
            flags = 0x08;
        pd.WriteUInt8(flags);
        pd.WritePackedGuid(characterGuidsLow[i], characterGuidsHigh[i]);
        pd.WriteUInt32(0);
    }
    session.SendPacket(pd);
}

void WritePackedSizes(PacketData@ pd, array<uint32>@ sizes, uint32 start, uint32 count)
{
    uint32 i = start;
    uint32 remaining = count;

    while (remaining > 0)
    {
        // Process up to 4 elements at a time
        uint32 chunk = remaining > 4 ? 4 : remaining;

        uint32 s0 = sizes[i];
        uint32 s1 = (chunk > 1) ? sizes[i + 1] : 0;
        uint32 s2 = (chunk > 2) ? sizes[i + 2] : 0;
        uint32 s3 = (chunk > 3) ? sizes[i + 3] : 0;

        // Build a 24-bit integer where high bits are filled first
        uint32 acc = ((s0 & 0x3F) << 18) |
                     ((s1 & 0x3F) << 12) |
                     ((s2 & 0x3F) << 6)  |
                      (s3 & 0x3F);

        // Determine how many raw bytes are needed to cover the values
        uint32 bytesToWrite = 0;
        if (chunk == 1) bytesToWrite = 1;      // Uses 6 bits -> 1 byte
        else if (chunk == 2) bytesToWrite = 2; // Uses 12 bits -> 2 bytes
        else bytesToWrite = 3;                 // Uses 18 or 24 bits -> 3 bytes

        // Write the bytes out Big-Endian to match parser's shifting window
        if (bytesToWrite >= 1) pd.WriteUInt8((acc >> 16) & 0xFF);
        if (bytesToWrite >= 2) pd.WriteUInt8((acc >> 8) & 0xFF);
        if (bytesToWrite >= 3) pd.WriteUInt8(acc & 0xFF);

        remaining -= chunk;
        i += chunk;
    }
}

void SendRegionwideCharacterMailData(WorldSession@ session, array<uint64>@ characterGuidsLow, array<uint64>@ characterGuidsHigh)
{
    Print("[RegionwideMail] Querying mail table...");
    // Join with characters table to get sender name (sender = character guid)
    QueryResult@ result = CharacterQuery(
        "SELECT m.id, m.receiver, c.name FROM mail m " +
        "LEFT JOIN characters c ON m.sender = c.guid " +
        "WHERE m.checked = 16");

    array<uint64> mailReceivers;
    array<string> mailSubjects;

    if (result !is null)
    {
        while (result.NextRow())
        {
            mailReceivers.insertLast(result.GetUInt64(1));
            mailSubjects.insertLast(result.GetString(2));
        }
    }
    Print("[RegionwideMail] Found " + mailReceivers.length() + " unread mails");

    uint32 charCount = characterGuidsLow.length();

    array<string> allSubjects;
    array<uint32> allSizes;
    array<uint32> charCounts;

    for (uint32 i = 0; i < charCount; i++)
    {
        uint64 guidLow = characterGuidsLow[i];
        uint32 cnt = 0;
        for (uint32 m = 0; m < mailReceivers.length(); m++)
        {
            if (mailReceivers[m] == guidLow)
            {
                string subj = mailSubjects[m];
                allSubjects.insertLast(subj);
                allSizes.insertLast(subj.length() + 1); // Length includes null terminator
                cnt++;
            }
        }
        charCounts.insertLast(cnt);
    }

    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_MAIL_DATA);
    pd.WriteUInt32(charCount);

    uint32 subjIdx = 0;
    for (uint32 i = 0; i < charCount; i++)
    {
        // Write exactly 1 byte.
        // Example: 0x01 means Type = 1, and TypeMask (1 >> 5) = 0.
        pd.WriteUInt8(1);

        // Follow up with standard Packed Guid block
        pd.WritePackedGuid(characterGuidsLow[i], characterGuidsHigh[i]);

        pd.WriteUInt32(charCounts[i]);    // MailEntryCount

        uint32 senderCount = (charCounts[i] > 0) ? 1 : 0;
        pd.WriteUInt32(senderCount);      // MailSenderCount
        if (senderCount > 0)
            pd.WriteUInt32(0);            // MailSenderType

        if (charCounts[i] > 0)
        {
            // Write the layout lengths
            WritePackedSizes(pd, allSizes, subjIdx, charCounts[i]);

            // Append the explicit string data streams
            for (uint32 j = 0; j < charCounts[i]; j++)
            {
                string s = allSubjects[subjIdx];
                uint32 len = s.length();
                for (uint32 k = 0; k < len; k++)
                    pd.WriteUInt8(s[k]);

                pd.WriteUInt8(0); // Explicitly finalize string boundary sequence
                subjIdx++;
            }
        }
    }

    Print("[RegionwideMail] Balanced byte alignment packet sent.");
    session.SendPacket(pd);
}
