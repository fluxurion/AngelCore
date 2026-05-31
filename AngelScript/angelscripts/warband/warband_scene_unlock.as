/*
 * warband_scene_unlock.as
 * Unlocks all WarbandScene.db2 entries for every player on login.
 * Loads WarbandScene.db2 once at startup via the Dynamic DB2 API,
 * then on PLAYER_ON_LOGIN writes any missing rows into
 * `battlenet_account_warband_scenes` (auth DB) so CollectionMgr
 * picks them up on the next session init (next login). To apply
 * immediately in the current session the packet
 * SMSG_ACCOUNT_WARBAND_SCENE_UPDATE is sent directly.
 */

#include "../includes/ScriptFramework.as"
#include "../db2/warband_scene_db2.as"

// SMSG_ACCOUNT_WARBAND_SCENE_UPDATE opcode (12.x)
const uint32 SMSG_ACCOUNT_WARBAND_SCENE_UPDATE = 0x420052;

void OnCharEnum(WorldSession@ session, EnumCharactersResult@ enumResult)
{
    if (session is null)
    {
        Print(AS_COLOR_RED + "[WarbandSceneUnlock] OnCharEnum: session is null" + AS_COLOR_RESET);
        return;
    }

    // Skip deleted-characters enum — only process the normal char list
    if (enumResult !is null && enumResult.IsDeletedCharacters())
        return;

    uint32 bnetId = session.GetBattlenetAccountId();
    string bnetStr = "" + bnetId;

    array<uint32> sceneIds = GetAllWarbandSceneIds();
    uint32 totalCount = sceneIds.length();

    Print(AS_COLOR_CYAN + "[WarbandSceneUnlock] OnCharEnum fired for account=" + bnetId + " sceneCount=" + totalCount + AS_COLOR_RESET);

    if (totalCount == 0)
    {
        Print(AS_COLOR_YELLOW + "[WarbandSceneUnlock] Skipping: No scene IDs found in DB2" + AS_COLOR_RESET);
        return;
    }

    // 1. Fetch currently owned scenes from DB
    AngelDBResult result = AngelDB_Query("SELECT `warbandSceneId` FROM `battlenet_account_warband_scenes` WHERE `battlenetAccountId` = " + bnetStr + " ORDER BY `warbandSceneId` ASC");

    array<uint32> ownedIds;
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            ownedIds.insertLast(result.GetUInt32(0));
        }
    }

    // 2. Identify missing scenes and insert them
    uint32 inserted = 0;
    for (uint32 i = 0; i < totalCount; i++)
    {
        uint32 sceneId = sceneIds[i];

        bool alreadyOwned = false;
        for (uint32 j = 0; j < ownedIds.length(); j++)
        {
            if (ownedIds[j] == sceneId) { alreadyOwned = true; break; }
        }

        if (!alreadyOwned)
        {
            AngelDB_Execute(
                "INSERT IGNORE INTO `battlenet_account_warband_scenes` (`battlenetAccountId`,`warbandSceneId`,`isFavorite`,`hasFanfare`) VALUES (" +
                bnetStr + "," + sceneId + ",0,0)"
            );
            inserted++;
        }
    }

    if (inserted > 0)
        Print(AS_COLOR_GREEN + "[WarbandSceneUnlock] Inserted " + inserted + " new scenes for account " + bnetId + AS_COLOR_RESET);

    // 3. Send SMSG_ACCOUNT_WARBAND_SCENE_UPDATE
    PacketData@ pkt = CreatePacketData(SMSG_ACCOUNT_WARBAND_SCENE_UPDATE);
    pkt.WriteBit(true);            // IsFullUpdate = true
    pkt.WriteUInt32(totalCount);   // count for IDs
    pkt.WriteUInt32(totalCount);   // count for favorites
    pkt.WriteUInt32(totalCount);   // count for fanfare

    for (uint32 i = 0; i < totalCount; i++)
        pkt.WriteUInt32(sceneIds[i]);

    for (uint32 i = 0; i < totalCount; i++)
        pkt.WriteBit(false);       // isFavorite

    for (uint32 i = 0; i < totalCount; i++)
        pkt.WriteBit(false);       // hasFanfare

    pkt.FlushBits();
    session.SendPacket(pkt);
    Print(AS_COLOR_GREEN + "[WarbandSceneUnlock] Packet sent to session" + AS_COLOR_RESET);
}

// ============================================================
// Registration
// ============================================================

void main()
{
    Print(AS_COLOR_CYAN + "[WarbandSceneUnlock] Script loaded" + AS_COLOR_RESET);
    RegisterWarbandSceneDB2();
    RegisterCharEnumHook(@OnCharEnum);
}
