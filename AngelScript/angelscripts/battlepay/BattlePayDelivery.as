/*
 * BattlePay Delivery Functions
 * Mail delivery, SQL delivery, and collection management
 */

#include "../includes/ScriptFramework.as"

// ============================================================================
// MAIL DELIVERY
// ============================================================================
bool DeliverViaMail(uint32 characterGuid, uint32 itemID, uint32 itemCount, uint32 accountID = 0)
{
    return DeliverViaMail(characterGuid, itemID, itemCount, "BattlePay Delivery", "Your purchased item has been delivered.");
}

bool DeliverViaMail(uint32 characterGuid, uint32 itemID, uint32 itemCount, string subject, string body)
{
    if (characterGuid == 0 || itemID == 0 || itemCount == 0) return false;

    uint32 mailId = 0;
    QueryResult@ result = CharacterQuery("SELECT MAX(id) + 1 FROM mail");
    if (result !is null && result.NextRow())
        mailId = result.GetUInt32(0);
    if (mailId == 0) mailId = 1;

    uint32 itemInstanceId = 0;
    @result = CharacterQuery("SELECT MAX(guid) + 1 FROM item_instance");
    if (result !is null && result.NextRow())
        itemInstanceId = result.GetUInt32(0);
    if (itemInstanceId == 0) itemInstanceId = 1;

    CharacterExecute(
        "INSERT INTO mail (id, messageType, stationery, sendermailTemplateId, sender, receiver, "
        "subject, body, has_items, expire_time, delivery_time, money, cod, checked) VALUES ("
        + mailId + ", 0, 1, 0, 0, " + characterGuid + ", '" + subject + "', '" + body + "', 1, "
        "DATE_ADD(NOW(), INTERVAL 30 DAY), NOW(), 0, 0, 0)");

    CharacterExecute(
        "INSERT INTO item_instance (guid, itemEntry, count) VALUES ("
        + itemInstanceId + ", " + itemID + ", " + itemCount + ")");

    CharacterExecute(
        "INSERT INTO mail_items (mail_id, item_guid, receiver) VALUES ("
        + mailId + ", " + itemInstanceId + ", " + characterGuid + ")");

    Print(AS_COLOR_GREEN + "[BattlePay] Delivered item " + itemID + " x" + itemCount + " via mail to character " + characterGuid + AS_COLOR_RESET);
    return true;
}

bool DeliverGoldViaMail(uint32 characterGuid, uint64 goldInCopper, uint32 accountID = 0)
{
    return DeliverGoldViaMail(characterGuid, goldInCopper, "BattlePay Delivery", "Your purchased gold has been delivered.");
}

bool DeliverGoldViaMail(uint32 characterGuid, uint64 goldInCopper, string subject, string body)
{
    if (characterGuid == 0 || goldInCopper == 0) return false;

    uint32 mailId = 0;
    QueryResult@ result = CharacterQuery("SELECT MAX(id) + 1 FROM mail");
    if (result !is null && result.NextRow())
        mailId = result.GetUInt32(0);
    if (mailId == 0) mailId = 1;

    CharacterExecute(
        "INSERT INTO mail (id, messageType, stationery, sendermailTemplateId, sender, receiver, "
        "subject, body, has_items, expire_time, delivery_time, money, cod, checked) VALUES ("
        + mailId + ", 0, 1, 0, 0, " + characterGuid + ", '" + subject + "', '" + body + "', 0, "
        "DATE_ADD(NOW(), INTERVAL 30 DAY), NOW(), " + goldInCopper + ", 0, 0)");

    Print(AS_COLOR_GREEN + "[BattlePay] Delivered " + goldInCopper + " copper via mail to character " + characterGuid + AS_COLOR_RESET);
    return true;
}

// Note: Delivery functions (DeliverItem, DeliverGold, DeliverMount, etc.) are in BattlePayStubs.as
