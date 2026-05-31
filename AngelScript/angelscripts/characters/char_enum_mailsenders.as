// ============================================================
// Character Enum Mail Senders
// Populates mail sender info on character selection screen
// ============================================================

#include "../includes/ScriptFramework.as"

// Hook into character enum to add mail sender data
void OnCharEnum(WorldSession@ session, EnumCharactersResult@ enumResult)
{
    if (enumResult is null || session is null)
        return;

    uint32 accountId = session.GetAccountId();
    if (accountId == 0)
        return;

    // Query mail data for all characters on this account
    // Matches the C++ CHAR_SEL_ENUM_MAILDATA prepared statement
    string query = "SELECT m.receiver, m.messageType, m.sender, cs.name "
                   "FROM mail m "
                   "INNER JOIN characters c ON c.guid = m.receiver "
                   "LEFT JOIN characters cs ON m.messageType = 0 AND cs.guid = m.sender "
                   "WHERE c.account = " + accountId +
                   " AND c.deleteInfos_Name IS NULL "
                   "AND m.deliver_time <= UNIX_TIMESTAMP() "
                   "AND m.expire_time > UNIX_TIMESTAMP() "
                   "AND (m.checked & 1) = 0 "
                   "ORDER BY m.receiver, m.id DESC";

    QueryResult@ result = CharacterQuery(query);
    if (result is null)
        return;

    do
    {
        uint64 charGuid = result.GetUInt64(0);
        uint32 senderType = result.GetUInt32(1);
        string senderName = result.GetString(3);

        CharEnumCharacterInfo@ charInfo = enumResult.FindCharacterByGuid(charGuid);
        if (charInfo !is null)
            charInfo.AddMailSender(senderName, senderType);
    }
    while (result.NextRow());
}

// ============================================================
// Main entry point
// ============================================================
void main()
{
    RegisterCharEnumHook(@OnCharEnum);
    Print("[CharEnum] Mail senders hook registered");
}
