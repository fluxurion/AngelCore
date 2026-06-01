// ============================================================
// Character Enum Mail Senders
// Handles CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA
// Sends SMSG_REGIONWIDE_CHARACTER_MAIL_DATA with actual mail data
// ============================================================

#include "../includes/ScriptFramework.as"
#include "RegionwideCharacterOpcodes.as"
#include "RegionwideCharacterPackets.as"

// ============================================================
// Handle client request for regionwide mail data
// Client sends this after receiving SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
// ============================================================
bool HandleRegionwideMailData(WorldSession@ session, PacketData@ packet)
{
    uint8 requestByte = packet.ReadUInt8();
    Print("[CharEnum] CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA byte: " + requestByte);

    uint32 accountId = session.GetAccountId();
    if (accountId == 0)
    {
        // Send empty response
        array<uint64> emptyLow;
        array<uint64> emptyHigh;
        SendRegionwideCharacterMailData(session, emptyLow, emptyHigh);
        return true;
    }

    // Build response packet
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_MAIL_DATA);

    // --- Step 1: Get all characters for this account ---
    array<uint64> charGuidsLow;
    array<uint64> charGuidsHigh;

    string charQuery = "SELECT guid FROM characters WHERE account = " + accountId + " ORDER BY guid";
    QueryResult@ charsResult = CharacterQuery(charQuery);

    if (charsResult !is null && charsResult.GetRowCount() > 0)
    {
        do
        {
            uint64 guid = charsResult.GetUInt64(0);
            uint64 guidLow, guidHigh;
            BuildPlayerGuid(guid, guidLow, guidHigh);
            charGuidsLow.insertLast(guidLow);
            charGuidsHigh.insertLast(guidHigh);
        }
        while (charsResult.NextRow());
    }

    uint32 charCount = charGuidsLow.length();

    // --- Step 2: Query unread mail for all characters ---
    // Groups mail by receiver guid so we can build per-character entries
    string mailQuery = "SELECT m.receiver, m.id, m.messageType, m.sender, cs.name, m.subject "
                       "FROM mail m "
                       "INNER JOIN characters c ON c.guid = m.receiver "
                       "LEFT JOIN characters cs ON m.messageType = 0 AND cs.guid = m.sender "
                       "WHERE c.account = " + accountId +
                       " AND c.deleteInfos_Name IS NULL "
                       "AND m.deliver_time <= UNIX_TIMESTAMP() "
                       "AND m.expire_time > UNIX_TIMESTAMP() "
                       "AND (m.checked & 1) = 0 "
                       "ORDER BY m.receiver, m.id DESC";

    QueryResult@ mailResult = CharacterQuery(mailQuery);

    // Group mail data by receiver guid
    // We'll collect arrays per character
    array<uint64> mailReceiverGuids;     // character guid for each mail entry group
    array<uint32> mailSenderCounts;       // number of senders for this char
    array<array<string>> mailSenders;     // sender names per char
    array<array<uint32>> mailSenderTypes; // sender types per char
    array<uint32> mailEntryCounts;        // number of mail entries for this char
    array<array<uint64>> mailEntryIdsLow; // mail entry guids per char
    array<array<uint64>> mailEntryIdsHigh;
    array<array<string>> mailSubjects;    // mail subjects per char

    uint64 currentReceiver = 0;
    int32 currentIndex = -1;

    if (mailResult !is null && mailResult.GetRowCount() > 0)
    {
        do
        {
            uint64 receiverGuid = mailResult.GetUInt64(0);
            uint64 mailId = mailResult.GetUInt64(1);
            uint32 messageType = mailResult.GetUInt32(2);
            string senderName = mailResult.GetString(4);
            string subject = mailResult.GetString(5);

            // New receiver group?
            if (receiverGuid != currentReceiver)
            {
                currentReceiver = receiverGuid;
                currentIndex++;
                mailReceiverGuids.insertLast(receiverGuid);
                mailSenderCounts.insertLast(0);
                mailSenders.insertLast(array<string>());
                mailSenderTypes.insertLast(array<uint32>());
                mailEntryCounts.insertLast(0);
                mailEntryIdsLow.insertLast(array<uint64>());
                mailEntryIdsHigh.insertLast(array<uint64>());
                mailSubjects.insertLast(array<string>());
            }

            // Add mail entry
            uint64 entryLow, entryHigh;
            BuildPlayerGuid(mailId, entryLow, entryHigh);
            mailEntryIdsLow[currentIndex].insertLast(entryLow);
            mailEntryIdsHigh[currentIndex].insertLast(entryHigh);
            mailSubjects[currentIndex].insertLast(subject);
            mailEntryCounts[currentIndex]++;

            // Add sender (messageType=0 means it's from a player, has a name)
            if (senderName != "")
            {
                mailSenders[currentIndex].insertLast(senderName);
                mailSenderTypes[currentIndex].insertLast(messageType);
                mailSenderCounts[currentIndex]++;
            }
        }
        while (mailResult.NextRow());
    }

    // --- Step 3: Write the packet ---
    pd.WriteUInt32(charCount);

    for (uint32 i = 0; i < charCount; i++)
    {
        uint8 type = 0;
        pd.WriteUInt8(type);
        pd.WritePackedGuid(charGuidsLow[i], charGuidsHigh[i]);

        // Find mail data for this character (if any)
        int32 mailIndex = -1;
        for (uint32 j = 0; j < mailReceiverGuids.length(); j++)
        {
            uint64 receiverLow, receiverHigh;
            BuildPlayerGuid(mailReceiverGuids[j], receiverLow, receiverHigh);
            if (receiverLow == charGuidsLow[i] && receiverHigh == charGuidsHigh[i])
            {
                mailIndex = int32(j);
                break;
            }
        }

        if (mailIndex >= 0)
        {
            // Write senders
            pd.WriteUInt32(mailSenderCounts[mailIndex]);
            for (uint32 s = 0; s < mailSenderCounts[mailIndex]; s++)
            {
                pd.WriteUInt32(mailSenderTypes[mailIndex][s]);
                pd.WriteCString(mailSenders[mailIndex][s]);
            }

            // Write mail entries
            pd.WriteUInt32(mailEntryCounts[mailIndex]);
            for (uint32 e = 0; e < mailEntryCounts[mailIndex]; e++)
            {
                pd.WritePackedGuid(mailEntryIdsLow[mailIndex][e], mailEntryIdsHigh[mailIndex][e]);
                pd.WriteCString(mailSubjects[mailIndex][e]);
            }
        }
        else
        {
            // No mail for this character
            pd.WriteUInt32(0);  // SenderCount = 0
            pd.WriteUInt32(0);  // EntryCount = 0
        }
    }

    session.SendPacket(pd);
    Print("[CharEnum] Sent SMSG_REGIONWIDE_CHARACTER_MAIL_DATA for " + charCount + " characters");
    return true;
}

// ============================================================
// Main entry point
// ============================================================
void main()
{
    RegisterOpcodeHandler(CMSG_GET_REGIONWIDE_CHARACTER_RESTRICTION_AND_MAIL_DATA, @HandleRegionwideMailData, false);
    Print("[CharEnum] Regionwide mail data handler registered");
}
