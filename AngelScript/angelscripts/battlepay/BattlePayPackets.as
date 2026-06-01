/*
 * BattlePay Packet Handlers
 * Updated to match WowPacketParser v12.0.5 structures
 */

#include "../includes/ScriptFramework.as"
#include "../Config.as"
#include "BattlePayOpcodes.as"
#include "BattlePayData.as"

// ============================================================================
// PACKET BUILDERS - Match WowPacketParser reading order
// ============================================================================
uint64 GeneratePurchaseID(uint32 accountID)
{
    uint64 timestamp = uint64(GetUnixTime());
    return (timestamp << 32) | uint64(accountID);
}

uint64 GenerateDistributionID(uint32 accountID)
{
    uint64 timestamp = uint64(GetUnixTime());
    return ((timestamp + 1) << 32) | uint64(accountID);
}

void SendStartPurchaseResponse(WorldSession@ session, uint32 result, uint64 purchaseID, uint32 checkoutToken)
{
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_START_PURCHASE_RESPONSE);
    pd.WriteUInt32(result);
    pd.WriteUInt64(purchaseID);
    pd.WriteUInt32(checkoutToken);
    pd.WriteUInt32(0);
    pd.WriteUInt32(0);

    session.SendPacket(pd);
}

void SendConfirmPurchase(WorldSession@ session, uint64 purchaseID, uint32 checkoutToken, uint32 result)
{
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_CONFIRM_PURCHASE);
    pd.WriteUInt64(purchaseID);
    pd.WriteUInt32(checkoutToken);
    pd.WriteUInt32(result);
    pd.WriteUInt32(0);
    pd.WriteUInt32(0);
    pd.WriteUInt32(0);

    session.SendPacket(pd);
}

void SendDistributionUpdate(WorldSession@ session, uint64 distributionID, uint64 purchaseID,
    uint32 status, uint32 productID, uint32 targetGuid, uint32 targetVirtualRealm,
    uint32 targetNativeRealm, uint32 charServiceFlags, uint32 customizationServiceType,
    uint32 requiredSourceGuid, uint32 requiredSourceVirtualRealm, uint32 requiredSourceNativeRealm,
    bool hasNewName)
{
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_DISTRIBUTION_UPDATE);

    pd.WriteBit(hasNewName);
    pd.WriteBit(false);
    pd.WriteBit(false);
    pd.FlushBits();

    pd.WriteUInt64(distributionID);
    pd.WriteUInt64(purchaseID);
    pd.WriteUInt32(status);
    pd.WriteUInt32(productID);
    pd.WriteUInt32(targetGuid);
    pd.WriteUInt32(targetVirtualRealm);
    pd.WriteUInt32(targetNativeRealm);
    pd.WriteUInt32(charServiceFlags);
    pd.WriteUInt32(customizationServiceType);
    pd.WriteUInt32(requiredSourceGuid);
    pd.WriteUInt32(requiredSourceVirtualRealm);
    pd.WriteUInt32(requiredSourceNativeRealm);

    if (hasNewName)
        pd.WriteString("NewName");

    session.SendPacket(pd);
}

// Write DisplayInfo - matches WowPacketParser ReadVisualMetadata v12.0.5 exactly
// Bit order: HasIconFileDataID, HasPreview, TitleLen(10), Title2Len(10),
//   DescLen(13), Desc2Len(13), Desc3Len(13), HasIconBorder, HasUnknown1,
//   HasUiTextureAtlasMemberID, HasUiTextureAtlasMemberID2, Desc4Len(13), Desc5Len(12)
// Then uint32s: VisualCount, CardType, Unknown3, ProductMultiplier
// Then conditional IconFileDataID[if HasIcon], conditional UIModelSceneID[if HasPreview]
// Then strings via WriteWoWString (raw bytes), then conditional IconBorder/Unknown1/Atlas1/Atlas2
// Then remaining strings, then visuals (bits nameLen(10) + flush + 3x uint32 + WriteWoWString name)
void WriteDisplayInfo(PacketData@ pd, DisplayInfo@ info)
{
    uint titleLen  = info !is null ? uint32(info.Title.length())       : 0;
    uint title2Len = info !is null ? uint32(info.Title2.length())      : 0;
    uint descLen   = info !is null ? uint32(info.Description.length())  : 0;
    uint desc2Len  = info !is null ? uint32(info.Description2.length()) : 0;
    uint desc3Len  = info !is null ? uint32(info.Description3.length()) : 0;
    uint desc4Len  = info !is null ? uint32(info.Description4.length()) : 0;
    uint desc5Len  = info !is null ? uint32(info.Description5.length()) : 0;

    bool hasIcon       = info !is null && info.HasIconFileDataID;
    bool hasPreview    = info !is null && info.HasPreview;
    bool hasIconBorder = info !is null && info.HasIconBorder;
    bool hasUnk1       = info !is null && info.HasUnknown1;
    bool hasAtlas1     = info !is null && info.HasUiTextureAtlasMemberID;
    bool hasAtlas2     = info !is null && info.HasUiTextureAtlasMemberID2;

    // === BIT PHASE - matches ReadVisualMetadata ReadBit/ReadBits order ===
    pd.WriteBit(hasIcon);
    pd.WriteBit(hasPreview);
    pd.WriteBits(titleLen, 10);
    pd.WriteBits(title2Len, 10);
    pd.WriteBits(descLen, 13);
    pd.WriteBits(desc2Len, 13);
    pd.WriteBits(desc3Len, 13);
    pd.WriteBit(hasIconBorder);
    pd.WriteBit(hasUnk1);
    pd.WriteBit(hasAtlas1);
    pd.WriteBit(hasAtlas2);
    pd.WriteBits(desc4Len, 13);
    pd.WriteBits(desc5Len, 12);
    pd.FlushBits();

    // === UINT32 PHASE (byte-aligned) ===
    pd.WriteUInt32(info !is null ? uint32(info.Visuals.length()) : 0);
    pd.WriteUInt32(info !is null ? info.CardType          : 0);
    pd.WriteUInt32(info !is null ? info.Unknown3          : 0);
    pd.WriteUInt32(info !is null ? info.ProductMultiplier : 0);

    // === CONDITIONAL UINT32s (first group) ===
    if (hasIcon)
        pd.WriteUInt32(info.IconFileDataID);

    // UIModelSceneID is conditional on HasPreview (matches parser)
    if (hasPreview)
        pd.WriteUInt32(info !is null ? info.UIModelSceneID : 0);

    // === STRINGS (first 5) - WriteWoWString writes raw bytes with known length ===
    pd.WriteWoWString(info !is null ? info.Title       : "", titleLen);
    pd.WriteWoWString(info !is null ? info.Title2      : "", title2Len);
    pd.WriteWoWString(info !is null ? info.Description  : "", descLen);
    pd.WriteWoWString(info !is null ? info.Description2 : "", desc2Len);
    pd.WriteWoWString(info !is null ? info.Description3 : "", desc3Len);

    // === CONDITIONAL UINT32s (second group - conditional on their respective bits) ===
    if (hasIconBorder)
        pd.WriteUInt32(info.IconBorder);

    if (hasUnk1)
        pd.WriteUInt32(info.Unknown1);

    if (hasAtlas1)
        pd.WriteUInt32(info.UiTextureAtlasMemberID);

    if (hasAtlas2)
        pd.WriteUInt32(info.UiTextureAtlasMemberID2);

    // === STRINGS (last 2) ===
    pd.WriteWoWString(info !is null ? info.Description4 : "", desc4Len);
    pd.WriteWoWString(info !is null ? info.Description5 : "", desc5Len);

    // === VISUALS ===
    if (info !is null)
    {
        for (uint v = 0; v < info.Visuals.length(); v++)
        {
            DisplayInfoVisual@ vis = info.Visuals[v];
            uint nameLen = uint32(vis.Name.length());

            pd.WriteBits(nameLen, 10);
            pd.FlushBits();

            pd.WriteUInt32(vis.CreatureDisplayID);
            pd.WriteUInt32(vis.PreviewUIModelSceneID);
            pd.WriteUInt32(vis.TransmogSetID);

            if (nameLen > 0)
                pd.WriteWoWString(vis.Name, nameLen);
        }
    }
}

// Write ProductInfo - matches ReadProductInfo in parser exactly
// Parser order: ProductID, NormalPrice, CurrentPrice,
//   deliverableCount, unknown1, unknown2, deliverableProductIDExtra,
//   deliverableCount2, unk1027, unkUInt64,
//   deliverableProducts[0..count], deliverableProducts2[0..count2],
//   hasDisplayByte, [VisualMetadata if bit7 set]
void WriteProductInfo(PacketData@ pd, ProductInfoEntry@ pi)
{
    pd.WriteUInt32(pi.ShopListingID);
    pd.WriteUInt64(pi.NormalPrice);
    pd.WriteUInt64(pi.CurrentPrice);

    pd.WriteUInt32(uint32(pi.DeliverableIDsArray.length())); // deliverableCount
    pd.WriteUInt32(pi.Unknown1);
    pd.WriteUInt32(pi.Unknown2);
    pd.WriteUInt32(pi.DeliverableIDExtra);
    pd.WriteUInt32(uint32(pi.DeliverableIDsArray2.length())); // deliverableCount2
    pd.WriteUInt32(pi.Unk1027);
    pd.WriteUInt64(pi.UnkUInt64);

    for (uint j = 0; j < pi.DeliverableIDsArray.length(); j++)
        pd.WriteUInt32(pi.DeliverableIDsArray[j]);
    for (uint j = 0; j < pi.DeliverableIDsArray2.length(); j++)
        pd.WriteUInt32(pi.DeliverableIDsArray2[j]);

    uint8 hasDisplayByte = pi.HasVisualMetadata ? 0x80 : 0;
    pd.WriteUInt8(hasDisplayByte);

    if (pi.HasVisualMetadata)
        WriteDisplayInfo(pd, pi.Display);
}

// Write Product — matches parser ReadProduct (line ~195)
// ReadUInt32: ProductID, Type, ItemID, ItemCount, MountSpellID, BattlePetSpeciesCreatureID,
//   Unknown1, Unknown2, Unknown3, TransmogSetID, Unknown8, Unknown9, Unknown10
// ReadByte: NameLength, FlagByte1, FlagByte2
// IF itemCountBits > 0: loop ReadProductItem
// ReadWoWString: Name
// IF hasDisplayInfo: ReadVisualMetadata
void WriteProduct(PacketData@ pd, ProductDataEntry@ prod)
{
    pd.WriteUInt32(prod.DeliverableID);              // ProductID
    pd.WriteUInt32(prod.Type);                        // Type
    pd.WriteUInt32(prod.ItemID);                      // ItemID
    pd.WriteUInt32(prod.ItemCount);                   // ItemCount
    pd.WriteUInt32(prod.MountSpellID);                // MountSpellID
    pd.WriteUInt32(prod.BattlePetSpeciesCreatureID);  // BattlePetSpeciesCreatureID
    pd.WriteUInt32(prod.Unknown1);                    // Unknown1
    pd.WriteUInt32(prod.Unknown2);                    // Unknown2
    pd.WriteUInt32(prod.Unknown3);                    // Unknown3
    pd.WriteUInt32(prod.TransmogSetID);               // TransmogSetID
    pd.WriteUInt32(prod.Unknown8);                    // Unknown8
    pd.WriteUInt32(prod.Unknown9);                    // Unknown9
    pd.WriteUInt32(prod.Unknown10);                   // Unknown10

    // NameLength
    string pName = (prod.Name != "") ? prod.Name : "";
    uint8 nameLen = uint8(pName.length());
    pd.WriteUInt8(nameLen);

    // FlagByte1: bit7 = AlreadyOwned, bit6 = hasPetSubFlag, bits0-5 = itemCountBits low
    uint8 flagByte1 = 0;
    // We don't support item sub-lists yet; keep itemCountBits = 0
    pd.WriteUInt8(flagByte1);

    // FlagByte2: bit7 = itemCountBits high, bit6 = hasDisplayInfo
    uint8 flagByte2 = 0;
    bool hasDisplay = prod.HasDisplayInfo && prod.Display !is null;
    if (hasDisplay)
        flagByte2 = 0x40;  // bit6 set
    pd.WriteUInt8(flagByte2);

    // No ReadProductItem loop (itemCountBits = 0)

    // Name bytes
    if (nameLen > 0)
        pd.WriteWoWString(pName, nameLen);

    // DisplayInfo
    if (hasDisplay)
        WriteDisplayInfo(pd, prod.Display);
}

// Write Group — matches updated parser ReadGroup
// ReadUInt32: GroupID, IconFileDataID
// ReadByte: DisplayType (uint8)
// ReadUInt32: Ordering, Unknown, MainGroupID
// ReadByte: NameLength (uint8, not bits!)
// ResetBitReader, ReadBits(DescriptionLength, 24)
// ReadWoWString: Name, Description
void WriteGroup(PacketData@ pd, GroupEntry@ g)
{
    pd.WriteUInt32(g.GroupID);
    pd.WriteUInt32(g.IconFileDataID);
    pd.WriteUInt8(uint8(g.DisplayType));
    pd.WriteUInt32(g.Ordering);
    pd.WriteUInt32(g.Unknown);
    pd.WriteUInt32(g.MainGroupID);

    // NameLength as uint8
    pd.WriteUInt8(uint8(g.Name.length()));

    // DescriptionLength as 24 bits
    pd.WriteBits(uint32(g.Description.length()), 24);
    pd.FlushBits();

    pd.WriteWoWString(g.Name, uint32(g.Name.length()));
    if (g.Description.length() > 0)
        pd.WriteWoWString(g.Description, uint32(g.Description.length()));
}

// Write Shop — matches updated parser ReadShop
// ReadUInt32: ShopFlags, Ordering, ProductID, GroupID, ShopListingID
// ReadByte: Field20, Flag (bit7=hasDisplayCard)
// if hasDisplayCard: ReadVisualMetadata
void WriteShop(PacketData@ pd, ShopEntry@ s)
{
    pd.WriteUInt32(s.ShopEntryID);       // ShopFlags
    pd.WriteUInt32(s.Ordering);          // Ordering
    pd.WriteUInt32(s.ShopListingID);     // ProductID
    pd.WriteUInt32(s.GroupID);           // GroupID
    pd.WriteUInt32(0);                   // ShopListingID (always 0 in retail)
    pd.WriteUInt8(uint8(s.StoreDeliveryType)); // Field20
    bool hasDisplayCard = s.HasBattlePayDisplayInfo && s.Display !is null;
    uint8 flag = hasDisplayCard ? 0x80 : 0;
    pd.WriteUInt8(flag);                 // Flag (bit7 = hasDisplayCard)
    if (hasDisplayCard)
        WriteDisplayInfo(pd, s.Display);
}

void SendPromotion(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_DISPLAY_PROMOTION);
    pd.WriteUInt32(0);
    session.SendPacket(pd);
}

void SendProductList(WorldSession@ session, bool autoOpen)
{
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE);

    // IDA order: Result (uint32), CurrencyID (uint32)
    pd.WriteUInt32(0); // Result = OK
    pd.WriteUInt32(CONFIG_BPAY_STORE_CURRENCY); // CurrencyID

    // Counts
    pd.WriteUInt32(uint32(g_productInfos.length()));  // ProductInfoCount
    pd.WriteUInt32(uint32(g_productDatas.length()));   // ProductCount
    pd.WriteUInt32(uint32(g_groups.length()));         // ProductGroupCount
    pd.WriteUInt32(uint32(g_shops.length()));          // ShopCount

    // Write product infos
    for (uint i = 0; i < g_productInfos.length(); i++)
    {
        ProductInfoEntry@ pi = g_productInfos[i];
        if (pi !is null)
            WriteProductInfo(pd, pi);
    }

    // Write products
    for (uint j = 0; j < g_productDatas.length(); j++)
    {
        ProductDataEntry@ prod = g_productDatas[j];
        if (prod !is null)
            WriteProduct(pd, prod);
    }

    // Write groups
    for (uint k = 0; k < g_groups.length(); k++)
    {
        GroupEntry@ g = g_groups[k];
        if (g !is null)
            WriteGroup(pd, g);
    }

    // Write shop entries
    for (uint s = 0; s < g_shops.length(); s++)
    {
        ShopEntry@ shop = g_shops[s];
        if (shop !is null)
            WriteShop(pd, shop);
    }

    Print(AS_COLOR_CYAN + "[BattlePay] Sent product list: " + g_productInfos.length() + " productInfos, " +
          g_productDatas.length() + " products, " + g_groups.length() + " groups, " +
          g_shops.length() + " shops" + AS_COLOR_RESET);
    session.SendPacket(pd);
}

// ============================================================================
// SendGenerateSSOTokenResponse — matches retail SMSG_GENERATE_SSO_TOKEN_RESPONSE (0x4202C5)
// Structure: uint32 Field32, uint32 Field36, uint64 TokenCreationTime, uint64 TokenExpirationTime,
//            uint8 StringLengthByte, bytes[Length] TokenString
// Sent after CMSG_BATTLE_PAY_OPEN_CHECKOUT to provide commerce checkout token
// ============================================================================
void SendGenerateSSOTokenResponse(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_GENERATE_SSO_TOKEN_RESPONSE);

    string token = "EU-4fab9214c0af02970faaaced7f90b398-401491545";
    uint32 tokenLen = uint32(token.length());

    pd.WriteUInt32(1);              // Field32 - result/status (1 = OK)
    pd.WriteUInt32(0);              // Field36
    pd.WriteUInt64(GetUnixTime());  // TokenCreationTime
    pd.WriteUInt64(GetUnixTime() + 14400);  // TokenExpirationTime (+4 hours)
    pd.WriteUInt8(tokenLen * 2);    // StringLengthByte = byte count (UTF-16 = chars * 2)
    pd.WriteWoWString(token, tokenLen); // TokenString as raw bytes

    session.SendPacket(pd);
}

// ============================================================================
// SendUnknownBeforeCharEnum — matches parser SMSG_UNKNOWN_BEFORE_CHAR_ENUM (0x420224)
// Structure: PackedGuid128(Guid) + ReadDisplayCard
// ============================================================================
void SendUnknownBeforeCharEnum(WorldSession@ session, uint64 guidLow)
{
    PacketData@ pd = CreatePacketData(SMSG_UNKNOWN_BEFORE_CHAR_ENUM);

    // WritePackedGuid128 - write as 2 uint64s (high + low parts)
    // guidLow is the low 64 bits, high part is 0 for player GUIDs
    pd.WriteUInt64(0);           // High part (type flags)
    pd.WriteUInt64(guidLow);     // Low part (GUID counter)

    // ReadDisplayCard = VisualMetadata = WriteDisplayInfo with null (empty/default)
    WriteDisplayInfo(pd, null);

    session.SendPacket(pd);
}

// ============================================================================
// SendDistributionList — updated parser: ReadUInt32(Result), ReadByte(Byte0), ReadByte(Byte1), count = (byte1>>5)|(8*byte0)
// ============================================================================
void SendDistributionList(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE);
    pd.WriteUInt32(0);  // Result = OK
    pd.WriteUInt8(0);   // Byte0
    pd.WriteUInt8(0);   // Byte1  (count = (0>>5)|(8*0) = 0)
    session.SendPacket(pd);
}

// ============================================================================
// SendVasPurchaseStates — matches parser: ResetBitReader, ReadBits(Result, 2)
// ============================================================================
void SendVasPurchaseStates(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_ENUM_VAS_PURCHASE_STATES_RESPONSE);
    pd.WriteBits(0, 2);  // Result = 0, 2 bits
    pd.FlushBits();
    session.SendPacket(pd);
}

// ============================================================================
// SendCurrencyUpdate — IDA: Count(uint32), [uint32, uint32, uint32] per entry
// ============================================================================
void SendCurrencyUpdate(WorldSession@ session, uint32 currencyID, uint64 balance)
{
    PacketData@ pd = CreatePacketData(SMSG_ACCOUNT_STORE_CURRENCY_UPDATE);
    pd.WriteUInt32(1);      // Count = 1 entry
    pd.WriteUInt32(currencyID);  // entry[0].uint32_0
    pd.WriteUInt32(0);           // entry[0].uint32_1
    pd.WriteUInt32(0);           // entry[0].uint32_2
    session.SendPacket(pd);
}

// ============================================================================
// SendSocialContractResponse — IDA: 1 byte (bit7 = ShowContract)
// ============================================================================
void SendSocialContractResponse(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_SOCIAL_CONTRACT_REQUEST_RESPONSE);
    pd.WriteUInt8(0);  // bit7 = 0 = don't show contract
    session.SendPacket(pd);
}

// ============================================================================
// SendStoreFrontInfoUpdate — responds to CMSG_REQUEST_STORE_FRONT_INFO_UPDATE
// IDA: uint8(flags), uint32(unk), array[uint32,uint32,uint32], uint8, array[32 bytes each]
// ============================================================================
void SendStoreFrontInfoUpdate(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_ACCOUNT_STORE_FRONT_UPDATE);
    pd.WriteUInt8(0);   // flags byte
    pd.WriteUInt32(0);  // unk uint32
    pd.WriteUInt32(0);  // array1 count = 0
    pd.WriteUInt32(0);  // array2 count = 0
    pd.WriteUInt8(0);   // sub-flags byte (bit7, bit6)
    session.SendPacket(pd);
}

void SendPurchaseList(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_BATTLE_PAY_GET_PURCHASE_LIST_RESPONSE);

    // parser: ReadUInt32(Result), ReadUInt32(PurchaseCount)
    pd.WriteUInt32(0); // Result = OK

    AngelDBResult result = AngelDB_Query(
        "SELECT id FROM battlepay_purchases WHERE account_id = " + session.GetAccountId() +
        " AND status < 2 ORDER BY created_at DESC LIMIT 50");

    array<uint64> purchaseIDs;
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
            purchaseIDs.insertLast(result.GetUInt64(0));
    }

    pd.WriteUInt32(uint32(purchaseIDs.length())); // PurchaseCount
    for (uint i = 0; i < purchaseIDs.length(); i++)
    {
        // Updated parser: PurchaseID(uint64), Unk1-3(uint32), Unk4-6(uint64), NameLen(uint8), Name(bytes)
        pd.WriteUInt64(purchaseIDs[i]);  // PurchaseID
        pd.WriteUInt32(0);               // Unk1
        pd.WriteUInt32(0);               // Unk2
        pd.WriteUInt32(0);               // Unk3
        pd.WriteUInt64(0);               // Unk4
        pd.WriteUInt64(0);               // Unk5
        pd.WriteUInt64(0);               // Unk6
        pd.WriteUInt8(0);                // NameLen = 0
        // No Name bytes
    }

    session.SendPacket(pd);
}

// ============================================================================
// SendDecorRefundListResponse — SMSG_GET_DECOR_REFUND_LIST_RESPONSE (0x420375)
// IDA Structure: uint32 count, array of {uint32, uint64, uint32, string, array}
// Simplified: empty list response (count = 0)
// ============================================================================
void SendDecorRefundListResponse(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_GET_DECOR_REFUND_LIST_RESPONSE);

    // Empty list response
    pd.WriteUInt32(0);  // count = 0

    session.SendPacket(pd);
}

// ============================================================================
// SendLicensedDecorQuantitiesResponse — SMSG_GET_ALL_LICENSED_DECOR_QUANTITIES_RESPONSE (0x42037A)
// IDA Structure: uint32 count, array of {uint32, uint32, uint32}
// Simplified: empty list response (count = 0)
// ============================================================================
void SendLicensedDecorQuantitiesResponse(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_GET_ALL_LICENSED_DECOR_QUANTITIES_RESPONSE);

    // Empty list response
    pd.WriteUInt32(0);  // count = 0

    session.SendPacket(pd);
}

// ============================================================================
// SendCommerceTokenMarketPriceResponse — SMSG_COMMERCE_TOKEN_GET_MARKET_PRICE_RESPONSE (0x42027B)
// IDA Structure: uint32 field32, uint32 field36, uint64 marketPrice, uint32 field48
// ============================================================================
void SendCommerceTokenMarketPriceResponse(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_COMMERCE_TOKEN_GET_MARKET_PRICE_RESPONSE);

    pd.WriteUInt32(0);   // field32
    pd.WriteUInt32(0);   // field36
    pd.WriteUInt64(0);   // marketPrice (in copper)
    pd.WriteUInt32(0);   // field48

    session.SendPacket(pd);
}

// ============================================================================
// SendConsumableTokenCanVeteranBuyResponse — SMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY_RESPONSE (0x42027F)
// IDA Structure: uint32 field32, uint32 field36, uint64 field40
// ============================================================================
void SendConsumableTokenCanVeteranBuyResponse(WorldSession@ session)
{
    PacketData@ pd = CreatePacketData(SMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY_RESPONSE);

    pd.WriteUInt32(0);   // field32
    pd.WriteUInt32(0);   // field36
    pd.WriteUInt64(0);   // field40

    session.SendPacket(pd);
}

// ============================================================================
// SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA
// Structure: Count + [Flags, Guid, RestrictionID] per character
// Flags: bit 5-7 = TopBits, bit 4 = IsRestricted, bit 3 = CatchUpAvailable
// ============================================================================
void SendRegionwideCharacterRestrictionsData(WorldSession@ session, array<ObjectGuid>@ characterGuids)
{
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_RESTRICTIONS_DATA);

    uint32 count = characterGuids.length();
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
        pd.WritePackedGuid128(characterGuids[i]);
        pd.WriteUInt32(0);  // RestrictionID = 0 (no restrictions)
    }

    session.SendPacket(pd);
}

// ============================================================================
// SMSG_REGIONWIDE_CHARACTER_MAIL_DATA
// Structure: Count + [Type(byte), Guid(packed128), SenderCount, Senders[], EntryCount, Entries[Guid, Subject]]
// Type: upper 3 bits of first byte
// For now: sending empty mail data (0 senders, 0 entries) as stub
// ============================================================================
void SendRegionwideCharacterMailData(WorldSession@ session, array<ObjectGuid>@ characterGuids)
{
    PacketData@ pd = CreatePacketData(SMSG_REGIONWIDE_CHARACTER_MAIL_DATA);

    uint32 count = characterGuids.length();
    pd.WriteUInt32(count);

    for (uint32 i = 0; i < count; i++)
    {
        uint8 type = 0;  // Type in upper 3 bits (0 for now)
        pd.WriteUInt8(type);
        pd.WritePackedGuid128(characterGuids[i]);

        // MailSenderCount = 0 (no senders)
        pd.WriteUInt32(0);

        // MailEntryCount = 0 (no mail entries)
        pd.WriteUInt32(0);
    }

    session.SendPacket(pd);
}
