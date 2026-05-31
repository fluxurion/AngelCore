/*
 * warband_groups.as
 *
 * AngelScript implementation of warband group saving.
 * REWRITTEN: Uses structured EnumCharactersResult API instead of raw packets.
 */

#include "../includes/Common.as"

const uint32 CMSG_SETUP_WARBAND_GROUPS = 0x40018F;
const uint32 MAX_WARBAND_GROUPS        = 20;
const uint32 MAX_WARBAND_MEMBERS       = 200;

// ===================================================================================
// WarbandGroup classes - used for CMSG_SETUP_WARBAND_GROUPS packet parsing only
// ===================================================================================

class WarbandGroupMember
{
    uint32 SlotIndex;
    int32  MemberType;
    int32  ContentSetID;
    uint64 GuidLow;

    void ReadFrom(PacketData@ packet)
    {
        SlotIndex    = packet.ReadUInt32();
        MemberType   = packet.ReadInt32();
        ContentSetID = packet.ReadInt32();

        if (MemberType == 0)
        {
            uint64 guidHigh;
            packet.ReadPackedGuid(GuidLow, guidHigh);
        }
    }
};

class WarbandGroup
{
    uint64 GroupID;
    uint8  OrderIndex;
    uint32 WarbandSceneID;
    uint32 Flags;
    int32  ContentSetID;
    array<WarbandGroupMember@> Members;
    string Name;

    void ReadFrom(PacketData@ packet)
    {
        GroupID        = packet.ReadUInt64();
        OrderIndex     = packet.ReadUInt8();
        WarbandSceneID = packet.ReadUInt32();
        Flags          = packet.ReadUInt32();
        ContentSetID   = packet.ReadInt32();

        uint32 memberCount = packet.ReadUInt32();
        if (memberCount > MAX_WARBAND_MEMBERS)
            memberCount = MAX_WARBAND_MEMBERS;

        Members.resize(memberCount);
        for (uint32 i = 0; i < memberCount; i++)
        {
            WarbandGroupMember@ member = WarbandGroupMember();
            member.ReadFrom(packet);
            @Members[i] = member;
        }

        uint32 nameLen = packet.ReadBits(9);
        packet.ResetBitReader();
        if (nameLen > 0)
            Name = packet.ReadWoWString(nameLen);
    }
};

// ===================================================================================

void RegisterHooks()
{
    RegisterOpcodeHandler(CMSG_SETUP_WARBAND_GROUPS, @HandleSetupWarbandGroups, true);
    RegisterCharEnumHook(@OnCharEnum);
}

// Note: CMSG_SETUP_WARBAND_GROUPS still uses raw PacketData - this is a separate handler
bool HandleSetupWarbandGroups(WorldSession@ session, PacketData@ packet)
{
    if (session is null || packet is null)
        return false;

    uint32 accountId = session.GetAccountId();

    uint32 groupCount = packet.ReadBits(5);
    packet.ResetBitReader();

    if (groupCount > MAX_WARBAND_GROUPS)
        return true;

    array<WarbandGroup@> groups;
    groups.resize(groupCount);
    bool payloadValid = true;

    for (uint32 g = 0; g < groupCount; g++)
    {
        uint32 posBefore = packet.GetReadPos();
        WarbandGroup@ group = WarbandGroup();
        group.ReadFrom(packet);

        if (packet.GetReadPos() <= posBefore)
        {
            payloadValid = false;
            break;
        }
        @groups[g] = group;
    }

    if (!payloadValid)
        return true;

    // --- Database Persistence ---
    string accountStr = "" + accountId;

    // Snapshot existing name -> groupId mapping BEFORE deleting, so we can
    // reuse stable groupIds for already-known group names and prevent the
    // client from assigning a new ID to a group that already has one in the DB.
    array<string>  existingNames;
    array<uint32>  existingSceneIds;
    array<uint64>  existingGroupIds;

    AngelDBResult existingGroups = AngelDB_Query(
        "SELECT `groupId`, `warbandSceneId`, `name` FROM `warband_groups` WHERE `accountId` = " + accountStr
    );
    if (existingGroups.GetRowCount() > 0)
    {
        while (existingGroups.NextRow())
        {
            existingGroupIds.insertLast(existingGroups.GetUInt64(0));
            existingSceneIds.insertLast(existingGroups.GetUInt32(1));
            existingNames.insertLast(existingGroups.GetString(2));
        }
    }

    AngelDB_Execute("DELETE FROM `warband_group_members` WHERE `accountId` = " + accountStr);
    AngelDB_Execute("DELETE FROM `warband_groups` WHERE `accountId` = " + accountStr);

    array<string> persistedGroupNames;
    array<uint32> persistedGroupSceneIds;

    for (uint32 g = 0; g < groupCount; g++)
    {
        WarbandGroup@ group = groups[g];

        // Sanitize name: remove any non-printable or invalid UTF-8 leading bytes (like the \x80 error)
        string sanitizedName = "";
        for (uint i = 0; i < group.Name.length(); i++)
        {
            uint8 c = group.Name[i];
            if (c >= 32 && c <= 126)
                sanitizedName += group.Name.substr(i, 1);
            else if (c >= 160) // Simple allowance for some extended chars, but avoiding \x80-\x9F range if possible
                sanitizedName += group.Name.substr(i, 1);
        }

        // Reject duplicates within this packet (same name+scene already queued for insert)
        bool duplicateName = false;
        for (uint32 i = 0; i < persistedGroupNames.length(); i++)
        {
            if (persistedGroupSceneIds[i] == group.WarbandSceneID && persistedGroupNames[i] == sanitizedName)
            {
                duplicateName = true;
                break;
            }
        }

        if (duplicateName)
            continue;

        // If this name+scene already existed in DB under a different groupId, reuse
        // the original groupId so the client cannot reassign identities.
        for (uint32 i = 0; i < existingNames.length(); i++)
        {
            if (existingSceneIds[i] == group.WarbandSceneID && existingNames[i] == sanitizedName
                && existingGroupIds[i] != group.GroupID)
            {
                group.GroupID = existingGroupIds[i];
                break;
            }
        }

        persistedGroupNames.insertLast(sanitizedName);
        persistedGroupSceneIds.insertLast(group.WarbandSceneID);

        string escapedName = DBEscapeString(sanitizedName);

        AngelDB_Execute(
            "INSERT IGNORE INTO `warband_groups` (`accountId`,`groupId`,`orderIndex`,`warbandSceneId`,`flags`,`name`) VALUES (" +
            accountStr + "," + group.GroupID + "," + group.OrderIndex + "," + group.WarbandSceneID + "," + group.Flags + "," + "'" + escapedName + "')"
        );

        for (uint32 m = 0; m < group.Members.length(); m++)
        {
            WarbandGroupMember@ member = group.Members[m];
            uint64 charGuid = (member.MemberType == 0) ? member.GuidLow : 0;

            AngelDB_Execute(
                "INSERT IGNORE INTO `warband_group_members` (`accountId`,`groupId`,`characterGuid`,`warbandScenePlacementId`,`type`) VALUES (" +
                accountStr + "," + group.GroupID + "," + charGuid + "," + member.SlotIndex + "," + member.MemberType + ")"
            );
        }
    }

    return true;
}

// ===================================================================================
// Auto-create Favorites group
// ===================================================================================

const uint32 FAVORITES_WARBAND_SCENE_ID = 29;
const string FAVORITES_GROUP_NAME       = "Favorites";
const uint32 FAVORITES_MAX_MEMBERS      = 4;

void EnsureFavoritesGroup(uint32 accountId)
{
    string accountStr = "" + accountId;

    // Check if a Favorites group already exists for this account
    AngelDBResult existCheck = AngelDB_Query(
        "SELECT `groupId` FROM `warband_groups` WHERE `accountId` = " + accountStr +
        " AND `name` = '" + FAVORITES_GROUP_NAME + "' AND `warbandSceneId` = " + FAVORITES_WARBAND_SCENE_ID + " LIMIT 1"
    );
    if (existCheck.GetRowCount() > 0)
        return; // already exists, nothing to do

    // Fetch top-4 most-played characters for this account
    QueryResult@ charsResult = CharacterQuery(
        "SELECT `guid` FROM `characters` WHERE `account` = " + accountStr +
        " AND `deleteDate` IS NULL ORDER BY `totaltime` DESC LIMIT " + FAVORITES_MAX_MEMBERS
    );
    if (charsResult is null)
        return; // no characters on account, skip

    // Generate a stable groupId: use a hash of accountId so it's deterministic
    // We shift by 32 bits and OR with a fixed marker to stay in uint64 range and avoid collisions with client IDs
    uint64 groupId = (uint64(accountId) << 20) | uint64(0xFAB0);

    // Ensure this groupId doesn't already exist (collision guard)
    AngelDBResult idCheck = AngelDB_Query(
        "SELECT 1 FROM `warband_groups` WHERE `accountId` = " + accountStr +
        " AND `groupId` = " + groupId + " LIMIT 1"
    );
    if (idCheck.GetRowCount() > 0)
        return; // already present under this id

    // Insert the Favorites group row (orderIndex 0 = first position)
    AngelDB_Execute(
        "INSERT IGNORE INTO `warband_groups` (`accountId`,`groupId`,`orderIndex`,`warbandSceneId`,`flags`,`name`) VALUES (" +
        accountStr + "," + groupId + ",0," + FAVORITES_WARBAND_SCENE_ID + ",0,'" + FAVORITES_GROUP_NAME + "')"
    );

    // Insert members at sequential slot indices
    uint32 slot = 0;
    do
    {
        uint64 charGuid = charsResult.GetUInt64(0);
        AngelDB_Execute(
            "INSERT IGNORE INTO `warband_group_members` (`accountId`,`groupId`,`characterGuid`,`warbandScenePlacementId`,`type`) VALUES (" +
            accountStr + "," + groupId + "," + charGuid + "," + slot + ",0)"
        );
        slot++;
    } while (charsResult.NextRow() && slot < FAVORITES_MAX_MEMBERS);

}

// ===================================================================================

void OnCharEnum(WorldSession@ session, EnumCharactersResult@ enumResult)
{
    if (session is null || enumResult is null)
        return;

    uint32 accountId = session.GetAccountId();

    EnsureFavoritesGroup(accountId);

    // Clear any existing warband groups
    enumResult.ClearWarbandGroups();

    AngelDBResult groupsResult = AngelDB_Query(
        "SELECT `groupId`, `orderIndex`, `warbandSceneId`, `flags`, `name` " +
        "FROM `warband_groups` WHERE `accountId` = " + accountId + " ORDER BY `orderIndex`"
    );

    if (groupsResult.GetRowCount() == 0)
        return;

    while (groupsResult.NextRow())
    {
        uint64 groupId        = groupsResult.GetUInt64(0);
        uint8  orderIndex     = uint8(groupsResult.GetUInt32(1));
        uint32 warbandSceneId = groupsResult.GetUInt32(2);
        uint32 flags          = groupsResult.GetUInt32(3);
        string rawName        = groupsResult.GetString(4);

        // Sanitize name
        string sanitizedName = "";
        for (uint i = 0; i < rawName.length(); i++)
        {
            uint8 c = rawName[i];
            if (c >= 32 && c <= 126)
                sanitizedName += rawName.substr(i, 1);
        }

        // Add warband group to enum result
        uint32 groupIndex = enumResult.GetWarbandGroupCount();
        enumResult.AddWarbandGroup(groupId, orderIndex, warbandSceneId, flags, 0, sanitizedName);

        // Query and add members
        AngelDBResult membersResult = AngelDB_Query(
            "SELECT `characterGuid`, `warbandScenePlacementId`, `type` " +
            "FROM `warband_group_members` WHERE `accountId` = " + accountId +
            " AND `groupId` = " + groupId
        );

        if (membersResult.GetRowCount() > 0)
        {
            while (membersResult.NextRow())
            {
                uint64 charGuid     = membersResult.GetUInt64(0);
                uint32 slotIndex    = membersResult.GetUInt32(1);
                int32  memberType   = int32(membersResult.GetUInt32(2));

                enumResult.AddWarbandGroupMember(groupIndex, slotIndex, memberType, 0, charGuid);
            }
        }
    }
}

// ===================================================================================

void main()
{
    Print(AS_COLOR_CYAN + "[Warband] Registering hooks..." + AS_COLOR_RESET);
    RegisterHooks();
    Print(AS_COLOR_CYAN + "[Warband] Hooks registered" + AS_COLOR_RESET);
}
