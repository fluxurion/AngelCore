/*
 * BattlePay Data Structures and Loading
 * Updated to match new WowPacketParser v12.0.5 schema (5/13/2026)
 * Schema: display_infos uses (SourceType, SourceID) composite PK
 *         product_infos uses ShopListingID/DeliverableIDs
 *         product_datas uses DeliverableID, Items embedded text column
 *         shop_datas uses ShopEntryID/ShopListingID
 */

// ============================================================================
// DATA STRUCTURES - Match WowPacketParser
// ============================================================================

class DisplayInfoVisual
{
    string Name;
    uint32 CreatureDisplayID;
    uint32 PreviewUIModelSceneID;
    uint32 TransmogSetID;
}

class DisplayInfo
{
    // Composite PK from new schema
    uint32 SourceType;  // 1=ProductInfo, 2=Product, 3=Shop, 4=ProductItem
    uint32 SourceID;    // FK depending on SourceType

    // Bit flags
    bool HasIconFileDataID;
    bool HasPreview;
    bool HasIconBorder;
    bool HasUnknown1;
    bool HasUiTextureAtlasMemberID;
    bool HasUiTextureAtlasMemberID2;

    // String lengths (for bit reading)
    uint32 TitleLength;
    uint32 Title2Length;
    uint32 DescriptionLength;
    uint32 Description2Length;
    uint32 Description3Length;
    uint32 Description4Length;
    uint32 Description5Length;

    // Data fields
    uint32 VisualCount;
    uint32 CardType;
    uint32 Unknown3;
    uint32 ProductMultiplier;
    uint32 IconFileDataID;
    uint32 UIModelSceneID;
    string Title;
    string Title2;
    string Description;
    string Description2;
    string Description3;
    uint32 IconBorder;
    uint32 Unknown1;
    uint32 UiTextureAtlasMemberID;
    uint32 UiTextureAtlasMemberID2;
    string Description4;
    string Description5;

    // Preview data (comma-separated from parser, parsed into Visuals)
    string PreviewCreatureDisplayIDs;
    string PreviewUIModelSceneIDs;
    string PreviewTransmogSets;
    string PreviewTitles;

    // Visuals array (parsed from comma-separated preview text fields)
    array<DisplayInfoVisual@> Visuals;
}

class ProductItemEntry
{
    uint32 ID;
    uint32 UnknownByte;
    uint32 ItemID;
    uint32 Quantity;
    uint32 UnknownInt1;
    uint32 UnknownInt2;
    // Flag byte fields
    bool IsPet;
    bool HasPetResult;
    bool HasPetSubFlag;
    uint32 PetResultFlags;
    bool HasVisualMetadata;
    uint32 PetResultVariable;
    // Reference to parent DeliverableID
    uint32 DeliverableID;
}

class ProductDataEntry
{
    uint32 Entry;
    uint32 DeliverableID;       // was ProductID
    uint32 Type;
    uint32 ItemID;
    uint32 ItemCount;
    uint32 MountSpellID;
    uint32 BattlePetSpeciesCreatureID;
    uint32 Unknown1;
    uint32 Unknown2;
    uint32 Unknown3;
    uint32 TransmogSetID;
    uint32 Unknown8;
    uint32 Unknown9;
    uint32 Unknown10;
    uint32 Unknown11;
    // Boost level (for character upgrade products) - old schema column removed, default 0
    uint32 BoostType;
    // Flag fields
    bool HasDisplayInfo;
    uint32 PetResultVariable;
    string Name;
    bool AlreadyOwned;
    uint32 DisplayFlag;
    // Parsed items (from Items text column)
    array<ProductItemEntry@> Items;
    DisplayInfo@ Display;
}

class ProductInfoEntry
{
    uint32 Entry;
    uint32 ShopListingID;           // was ProductInfoID
    uint64 NormalPrice;
    uint64 CurrentPrice;
    uint32 ProductInfoFlags;
    uint32 Unknown1;
    uint32 Unknown2;
    uint32 Unknown3;
    uint32 Unknown4;
    uint32 Unknown5;
    uint32 DeliverableIDExtra;      // was DeliverableProductIDExtra
    uint32 Unk1027;
    uint64 UnkUInt64;
    // Flag fields from parser
    uint32 UnknownIfFlags1_1;
    uint32 UnknownIfFlags1_2;
    uint32 UnknownIfFlags2_1;
    uint32 UnknownIfFlags2_2;
    uint32 UnknownIfFlags2_3;
    uint32 UnknownIfFlags2_4;
    bool HasVisualMetadata;
    string DeliverableIDs;          // was DeliverableProductIDs
    string DeliverableIDs2;         // was DeliverableProductIDs2
    uint32 DisplayFlag;
    uint32 HasUnknown1InDisplayInfo;
    uint32 HasBattlePayDisplayInfo;
    uint32 ChoiceType;
    // Parsed arrays
    array<uint32> DeliverableIDsArray;      // was DeliverableProductIDsArray
    array<uint32> DeliverableIDsArray2;     // was DeliverableProductIDsArray2
    DisplayInfo@ Display;
}

class GroupEntry
{
    uint32 Entry;
    uint32 GroupID;
    uint32 IconFileDataID;
    uint32 DisplayType;
    uint32 Ordering;
    uint32 Unknown;
    uint32 MainGroupID;
    string Name;
    string Description;
}

class ShopEntry
{
    uint32 Entry;
    uint32 ShopEntryID;     // was EntryID
    uint32 GroupID;
    uint32 ShopListingID;   // was ProductID
    uint32 Ordering;
    uint32 VasServiceType;
    uint32 StoreDeliveryType;
    uint32 DisplayFlag;
    bool HasVisualMetadata;
    bool HasBattlePayDisplayInfo;
    uint32 Unknown;
    DisplayInfo@ Display;
}

// ============================================================================
// GLOBAL CACHE
// ============================================================================
array<uint32> g_disabledProducts;
array<GroupEntry@> g_groups;
array<ShopEntry@> g_shops;
array<ProductInfoEntry@> g_productInfos;
array<ProductDataEntry@> g_productDatas;
array<ProductItemEntry@> g_productItems;
array<DisplayInfo@> g_displayInfos;
bool g_dataLoaded = false;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
string GetProductName(uint32 nameID)
{
    AngelDBResult result = AngelDB_Query("SELECT Name FROM battlepay_product_names WHERE ID = " + nameID);
    if (result.GetRowCount() > 0 && result.NextRow())
        return result.GetString(0);
    return "Unknown Product";
}

uint32 GetAccountCredits(uint32 accountID)
{
    AngelDBResult result = AngelDB_Query("SELECT credits FROM battlepay_credits WHERE account_id = " + accountID);
    if (result.GetRowCount() > 0 && result.NextRow())
        return result.GetUInt32(0);
    return 0;
}

bool SetAccountCredits(uint32 accountID, uint32 credits)
{
    AngelDB_Execute("INSERT INTO battlepay_credits (account_id, credits) VALUES (" + accountID + ", " + credits + ") ON DUPLICATE KEY UPDATE credits = " + credits);
    return true;
}

bool HasEnoughCredits(uint32 accountID, uint32 cost)
{
    return GetAccountCredits(accountID) >= cost;
}

bool DeductCredits(uint32 accountID, uint32 amount)
{
    uint32 current = GetAccountCredits(accountID);
    if (current < amount)
        return false;
    return SetAccountCredits(accountID, current - amount);
}

// ============================================================================
// PARSING HELPERS
// ============================================================================

/// Parse comma-separated string into array of uint32
array<uint32> ParseCommaSeparatedUInt32(const string& in str)
{
    array<uint32> result;
    if (str == "") return result;
    array<string> parts = str.split(",");
    for (uint i = 0; i < parts.length(); i++)
    {
        string trimmed = parts[i];
        if (trimmed != "")
            result.insertLast(uint32(parseInt(trimmed)));
    }
    return result;
}

/// Parse the Items text column into ProductItemEntry objects
/// Format per item: ID,UnknownByte,ItemID,Quantity,UnknownInt1,UnknownInt2,IsPet,HasPetResult,PetResultFlags,HasVisualMetadata
/// Items separated by colon
array<ProductItemEntry@> ParseItemsText(const string& in itemsText, uint32 deliverableID)
{
    array<ProductItemEntry@> result;
    if (itemsText == "" || itemsText == "UNKNOWN") return result;

    array<string> items = itemsText.split(":");
    for (uint i = 0; i < items.length(); i++)
    {
        array<string> fields = items[i].split(",");
        if (fields.length() < 10) continue;

        ProductItemEntry@ item = ProductItemEntry();
        item.ID = uint32(parseInt(fields[0]));
        item.UnknownByte = uint32(parseInt(fields[1]));
        item.ItemID = uint32(parseInt(fields[2]));
        item.Quantity = uint32(parseInt(fields[3]));
        item.UnknownInt1 = uint32(parseInt(fields[4]));
        item.UnknownInt2 = uint32(parseInt(fields[5]));
        item.IsPet = parseInt(fields[6]) != 0;
        item.HasPetResult = parseInt(fields[7]) != 0;
        item.PetResultFlags = uint32(parseInt(fields[8]));
        item.HasVisualMetadata = parseInt(fields[9]) != 0;
        item.DeliverableID = deliverableID;
        result.insertLast(item);
    }
    return result;
}

/// Parse the comma-separated preview text fields into DisplayInfoVisual array
/// NOTE: Visual names (PreviewTitles) can contain commas (e.g. "Shu-Zen, the Divine Sentinel")
/// which breaks simple comma-splitting. We handle this by detecting the mismatch.
array<DisplayInfoVisual@> ParseVisualsFromPreview(const string& in creatureIDs, const string& in sceneIDs,
    const string& in transmogSets, const string& in titles, uint32 visualCount)
{
    array<DisplayInfoVisual@> result;

    array<uint32> creatureIDArr = ParseCommaSeparatedUInt32(creatureIDs);
    array<uint32> sceneIDArr = ParseCommaSeparatedUInt32(sceneIDs);
    array<uint32> transmogSetArr = ParseCommaSeparatedUInt32(transmogSets);

    // Collect visual names - handle the case where names contain commas
    array<string> titleParts;
    if (titles != "")
    {
        titleParts = titles.split(",");
        // FIX: If split produced more parts than VisualCount, some names contain commas.
        // When VisualCount==1, use the entire unsplit string as the single name.
        // When VisualCount>1, we can't reliably split since commas are both separators and part of names.
        if (titleParts.length() > visualCount && visualCount == 1)
        {
            // Names contain embedded commas - use the full raw string
            titleParts.resize(0);
            titleParts.insertLast(titles);
        }
        // Note: For VisualCount > 1 with comma-containing names, the data format is ambiguous.
        // In retail data, multi-visual products use simple names without commas, so this works.
    }

    uint32 count = visualCount;
    if (count == 0) return result;

    for (uint i = 0; i < count; i++)
    {
        DisplayInfoVisual@ vis = DisplayInfoVisual();
        vis.CreatureDisplayID = i < creatureIDArr.length() ? creatureIDArr[i] : 0;
        vis.PreviewUIModelSceneID = i < sceneIDArr.length() ? sceneIDArr[i] : 0;
        vis.TransmogSetID = i < transmogSetArr.length() ? transmogSetArr[i] : 0;
        vis.Name = i < titleParts.length() ? titleParts[i] : "";
        result.insertLast(vis);
    }
    return result;
}

// ============================================================================
// DATA LOADING - Match new SQL table structures
// ============================================================================

bool LoadBattlePayData()
{
    if (g_dataLoaded) return true;

    // Sanity check
    AngelDBResult sanityCheck = AngelDB_Query("SELECT COUNT(*) FROM battlepay_display_infos");
    if (sanityCheck.GetRowCount() == 0)
        PrintError("[BattlePay] display_infos table missing or empty in AngelDB!");

    // Load disabled products
    AngelDBResult result = AngelDB_Query("SELECT ProductID FROM battlepay_disabled_products");
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
            g_disabledProducts.insertLast(result.GetUInt32(0));
    }

    // Load groups - unchanged schema
    result = AngelDB_Query("SELECT Entry, GroupID, IconFileDataID, DisplayType, Ordering, Unknown, MainGroupID, Name, Description FROM battlepay_groups ORDER BY Ordering");
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            GroupEntry@ g = GroupEntry();
            g.Entry = result.GetUInt32(0);
            g.GroupID = result.GetUInt32(1);
            g.IconFileDataID = result.GetUInt32(2);
            g.DisplayType = result.GetUInt32(3);
            g.Ordering = result.GetUInt32(4);
            g.Unknown = result.GetUInt32(5);
            g.MainGroupID = result.GetUInt32(6);
            g.Name = result.GetString(7);
            g.Description = result.GetString(8);
            g_groups.insertLast(g);
        }
    }

    // Load shops - new columns: ShopFlags, ProductID, ShopListingID, Field20
    result = AngelDB_Query("SELECT Entry, ShopFlags, GroupID, ProductID, Ordering, ShopListingID, Field20, HasBattlePayDisplayInfo, Unknown, DisplayFlag FROM battlepay_shop_datas ORDER BY Entry");
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            ShopEntry@ s = ShopEntry();
            s.Entry = result.GetUInt32(0);
            s.ShopEntryID = result.GetUInt32(1);   // ShopFlags
            s.GroupID = result.GetUInt32(2);
            s.ShopListingID = result.GetUInt32(3);  // ProductID
            s.Ordering = result.GetUInt32(4);
            s.VasServiceType = result.GetUInt32(5); // ShopListingID
            s.StoreDeliveryType = result.GetUInt32(6); // Field20
            s.HasBattlePayDisplayInfo = result.GetUInt32(7) != 0;
            s.Unknown = result.GetUInt32(8);
            s.DisplayFlag = result.GetUInt32(9);
            s.HasVisualMetadata = s.HasBattlePayDisplayInfo;
            g_shops.insertLast(s);
        }
    }

    // Load product infos - new columns: ShopListingID, DeliverableIDs
    result = AngelDB_Query("SELECT Entry, ShopListingID, NormalPrice, CurrentPrice, ProductInfoFlags, Unknown1, Unknown2, Unknown3, Unknown4, Unknown5, DeliverableIDExtra, Unk1027, UnkUInt64, UnknownIfFlags1_1, UnknownIfFlags1_2, UnknownIfFlags2_1, UnknownIfFlags2_2, UnknownIfFlags2_3, UnknownIfFlags2_4, HasVisualMetadata, DeliverableIDs, DeliverableIDs2, DisplayFlag, HasUnknown1InDisplayInfo, HasBattlePayDisplayInfo, ChoiceType FROM battlepay_product_infos");
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            ProductInfoEntry@ pi = ProductInfoEntry();
            pi.Entry = result.GetUInt32(0);
            pi.ShopListingID = result.GetUInt32(1);
            pi.NormalPrice = result.GetUInt64(2);
            pi.CurrentPrice = result.GetUInt64(3);
            pi.ProductInfoFlags = result.GetUInt32(4);
            pi.Unknown1 = result.GetUInt32(5);
            pi.Unknown2 = result.GetUInt32(6);
            pi.Unknown3 = result.GetUInt32(7);
            pi.Unknown4 = result.GetUInt32(8);
            pi.Unknown5 = result.GetUInt32(9);
            pi.DeliverableIDExtra = result.GetUInt32(10);
            pi.Unk1027 = result.GetUInt32(11);
            pi.UnkUInt64 = result.GetUInt64(12);
            pi.UnknownIfFlags1_1 = result.GetUInt32(13);
            pi.UnknownIfFlags1_2 = result.GetUInt32(14);
            pi.UnknownIfFlags2_1 = result.GetUInt32(15);
            pi.UnknownIfFlags2_2 = result.GetUInt32(16);
            pi.UnknownIfFlags2_3 = result.GetUInt32(17);
            pi.UnknownIfFlags2_4 = result.GetUInt32(18);
            pi.HasVisualMetadata = result.GetUInt32(19) != 0;
            pi.DeliverableIDs = result.GetString(20);
            pi.DeliverableIDs2 = result.GetString(21);
            pi.DisplayFlag = result.GetUInt32(22);
            pi.HasUnknown1InDisplayInfo = result.GetUInt32(23);
            pi.HasBattlePayDisplayInfo = result.GetUInt32(24);
            pi.ChoiceType = result.GetUInt32(25);

            // Parse deliverable IDs (comma-separated)
            pi.DeliverableIDsArray = ParseCommaSeparatedUInt32(pi.DeliverableIDs);
            pi.DeliverableIDsArray2 = ParseCommaSeparatedUInt32(pi.DeliverableIDs2);

            g_productInfos.insertLast(pi);
        }
    }

    // Load product datas - new columns: DeliverableID, Items (text column)
    result = AngelDB_Query("SELECT Entry, DeliverableID, Type, ItemID, ItemCount, MountSpellID, BattlePetSpeciesCreatureID, Unknown1, Unknown2, Unknown3, TransmogSetID, Unknown8, Unknown9, Unknown10, Unknown11, HasDisplayInfo, PetResultVariable, Name, AlreadyOwned, DisplayFlag, Items FROM battlepay_product_datas");
    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            ProductDataEntry@ pd = ProductDataEntry();
            pd.Entry = result.GetUInt32(0);
            pd.DeliverableID = result.GetUInt32(1);
            pd.Type = result.GetUInt32(2);
            pd.ItemID = result.GetUInt32(3);
            pd.ItemCount = result.GetUInt32(4);
            pd.MountSpellID = result.GetUInt32(5);
            pd.BattlePetSpeciesCreatureID = result.GetUInt32(6);
            pd.Unknown1 = result.GetUInt32(7);
            pd.Unknown2 = result.GetUInt32(8);
            pd.Unknown3 = result.GetUInt32(9);
            pd.TransmogSetID = result.GetUInt32(10);
            pd.Unknown8 = result.GetUInt32(11);
            pd.Unknown9 = result.GetUInt32(12);
            pd.Unknown10 = result.GetUInt32(13);
            pd.Unknown11 = result.GetUInt32(14);
            pd.BoostType = 0; // Column removed from new schema, default to 0
            pd.HasDisplayInfo = result.GetUInt32(15) != 0;
            pd.PetResultVariable = result.GetUInt32(16);
            pd.Name = result.GetString(17);
            pd.AlreadyOwned = result.GetUInt32(18) != 0;
            pd.DisplayFlag = result.GetUInt32(19);

            // Parse Items text column
            string itemsText = result.GetString(20);
            array<ProductItemEntry@> parsedItems = ParseItemsText(itemsText, pd.DeliverableID);
            for (uint i = 0; i < parsedItems.length(); i++)
            {
                pd.Items.insertLast(parsedItems[i]);
                g_productItems.insertLast(parsedItems[i]);
            }

            g_productDatas.insertLast(pd);
        }
    }

    // ========================================================================
    // BATCH LOAD ALL DISPLAY INFOS (new schema: SourceType, SourceID)
    // ========================================================================
    Print(AS_COLOR_CYAN + "[BattlePay] Loading all display_infos in batch (new schema)..." + AS_COLOR_RESET);
    result = AngelDB_Query("SELECT SourceType, SourceID, "
        + "HasIconFileDataID, HasPreview, HasIconBorder, HasUnknown1, "
        + "HasUiTextureAtlasMemberID, HasUiTextureAtlasMemberID2, "
        + "VisualCount, CardType, Unknown3, ProductMultiplier, "
        + "IconFileDataID, UIModelSceneID, "
        + "Title, Title2, Description, Description2, Description3, "
        + "IconBorder, Unknown1, "
        + "UiTextureAtlasMemberID, UiTextureAtlasMemberID2, "
        + "Description4, Description5, "
        + "PreviewCreatureDisplayIDs, PreviewUIModelSceneIDs, PreviewTransmogSets, PreviewTitles "
        + "FROM battlepay_display_infos ORDER BY SourceType, SourceID");

    if (result.GetRowCount() > 0)
    {
        while (result.NextRow())
        {
            DisplayInfo@ info = DisplayInfo();
            int col = 0;
            info.SourceType = result.GetUInt32(col++);
            info.SourceID = result.GetUInt32(col++);
            info.HasIconFileDataID = result.GetUInt32(col++) != 0;
            info.HasPreview = result.GetUInt32(col++) != 0;
            info.HasIconBorder = result.GetUInt32(col++) != 0;
            info.HasUnknown1 = result.GetUInt32(col++) != 0;
            info.HasUiTextureAtlasMemberID = result.GetUInt32(col++) != 0;
            info.HasUiTextureAtlasMemberID2 = result.GetUInt32(col++) != 0;
            info.VisualCount = result.GetUInt32(col++);
            info.CardType = result.GetUInt32(col++);
            info.Unknown3 = result.GetUInt32(col++);
            info.ProductMultiplier = result.GetUInt32(col++);
            info.IconFileDataID = result.GetUInt32(col++);
            info.UIModelSceneID = result.GetUInt32(col++);
            info.Title = result.GetString(col++);
            info.Title2 = result.GetString(col++);
            info.Description = result.GetString(col++);
            info.Description2 = result.GetString(col++);
            info.Description3 = result.GetString(col++);
            info.IconBorder = result.GetUInt32(col++);
            info.Unknown1 = result.GetUInt32(col++);
            info.UiTextureAtlasMemberID = result.GetUInt32(col++);
            info.UiTextureAtlasMemberID2 = result.GetUInt32(col++);
            info.Description4 = result.GetString(col++);
            info.Description5 = result.GetString(col++);
            info.PreviewCreatureDisplayIDs = result.GetString(col++);
            info.PreviewUIModelSceneIDs = result.GetString(col++);
            info.PreviewTransmogSets = result.GetString(col++);
            info.PreviewTitles = result.GetString(col++);

            // Parse visuals from comma-separated preview text fields
            info.Visuals = ParseVisualsFromPreview(
                info.PreviewCreatureDisplayIDs,
                info.PreviewUIModelSceneIDs,
                info.PreviewTransmogSets,
                info.PreviewTitles,
                info.VisualCount);

            g_displayInfos.insertLast(info);
        }
        Print("[BattlePay] Loaded " + g_displayInfos.length() + " display_infos");
    }
    else
        PrintError("[BattlePay] AngelDB returned no rows for display_infos batch load!");

    // ========================================================================
    // LINK DISPLAYS - SourceType-based matching
    // SourceType 1 → SourceID matches product_infos.ShopListingID
    // SourceType 3 → SourceID matches shop_datas.ShopEntryID
    // SourceType 2 → SourceID matches product_datas.DeliverableID
    // ========================================================================

    // Build lookup maps for efficiency
    // Map: SourceType=1,SourceID → DisplayInfo
    array<DisplayInfo@> displayByProductInfoID;
    // Map: SourceType=3,SourceID → DisplayInfo
    array<DisplayInfo@> displayByShopEntryID;
    // Map: SourceType=2,SourceID → DisplayInfo
    array<DisplayInfo@> displayByDeliverableID;

    for (uint i = 0; i < g_displayInfos.length(); i++)
    {
        DisplayInfo@ info = g_displayInfos[i];
        if (info.SourceType == 1)
            displayByProductInfoID.insertLast(info);
        else if (info.SourceType == 3)
            displayByShopEntryID.insertLast(info);
        else if (info.SourceType == 2)
            displayByDeliverableID.insertLast(info);
    }

    // ========================================================================
    // STEP 1: Link display info to product infos (SourceType=1 → ShopListingID)
    // ========================================================================
    for (uint i = 0; i < g_productInfos.length(); i++)
    {
        if (g_productInfos[i].HasBattlePayDisplayInfo != 0)
        {
            for (uint d = 0; d < displayByProductInfoID.length(); d++)
            {
                if (displayByProductInfoID[d].SourceID == g_productInfos[i].ShopListingID)
                {
                    @g_productInfos[i].Display = displayByProductInfoID[d];
                    break;
                }
            }
            if (g_productInfos[i].Display is null)
                PrintWarn("[BattlePay] ProductInfo Entry=" + g_productInfos[i].Entry + " has no display (ShopListingID=" + g_productInfos[i].ShopListingID + ")");
        }
    }

    // ========================================================================
    // STEP 2: Link display info to shops (SourceType=3 → ShopEntryID)
    // Route A: Direct ShopEntryID lookup (SourceType=3, SourceID = shop.ShopEntryID)
    // Route B: Inherit from linked ProductInfo via shop.ShopListingID == product_info.ShopListingID
    // ========================================================================
    for (uint i = 0; i < g_shops.length(); i++)
    {
        if (!g_shops[i].HasBattlePayDisplayInfo) continue;

        // Route A: Direct display via SourceType=3, SourceID = shop.ShopEntryID
        for (uint d = 0; d < displayByShopEntryID.length(); d++)
        {
            if (displayByShopEntryID[d].SourceID == g_shops[i].ShopEntryID)
            {
                @g_shops[i].Display = displayByShopEntryID[d];
                break;
            }
        }

        // Route B: Inherit from product_info via ShopListingID
        if (g_shops[i].Display is null)
        {
            for (uint j = 0; j < g_productInfos.length(); j++)
            {
                if (g_productInfos[j].ShopListingID == g_shops[i].ShopListingID)
                {
                    @g_shops[i].Display = g_productInfos[j].Display;
                    break;
                }
            }
        }

        // Route C: Direct SourceType=2 lookup via DeliverableID (for product-level displays)
        if (g_shops[i].Display is null)
        {
            for (uint d = 0; d < displayByDeliverableID.length(); d++)
            {
                // Find product_datas linked to this shop's product_info
                for (uint j = 0; j < g_productInfos.length(); j++)
                {
                    if (g_productInfos[j].ShopListingID == g_shops[i].ShopListingID)
                    {
                        // Check if any deliverable in this product_info has a direct display
                        for (uint k = 0; k < g_productInfos[j].DeliverableIDsArray.length(); k++)
                        {
                            if (displayByDeliverableID[d].SourceID == g_productInfos[j].DeliverableIDsArray[k])
                            {
                                @g_shops[i].Display = displayByDeliverableID[d];
                                break;
                            }
                        }
                        if (g_shops[i].Display !is null) break;
                    }
                }
                if (g_shops[i].Display !is null) break;
            }
        }

        // Final status
        if (g_shops[i].Display is null)
            PrintWarn("[BattlePay] Shop Entry=" + g_shops[i].Entry + " has no display (ShopEntryID=" + g_shops[i].ShopEntryID + ")");
    }

    // ========================================================================
    // STEP 3: Link display info to product datas via parent ProductInfo
    // ProductInfo.DeliverableIDsArray contains ProductData.DeliverableID values
    // If a product_data has HasDisplayInfo, look for SourceType=2 display
    // Otherwise inherit from parent ProductInfo's display
    // ========================================================================
    for (uint i = 0; i < g_productDatas.length(); i++)
    {
        // First check for direct SourceType=2 display
        if (g_productDatas[i].HasDisplayInfo)
        {
            for (uint d = 0; d < displayByDeliverableID.length(); d++)
            {
                if (displayByDeliverableID[d].SourceID == g_productDatas[i].DeliverableID)
                {
                    @g_productDatas[i].Display = displayByDeliverableID[d];
                    break;
                }
            }
        }

        // If still no display, inherit from parent ProductInfo
        if (g_productDatas[i].Display is null)
        {
            for (uint j = 0; j < g_productInfos.length(); j++)
            {
                ProductInfoEntry@ pi = g_productInfos[j];
                for (uint k = 0; k < pi.DeliverableIDsArray.length(); k++)
                {
                    if (pi.DeliverableIDsArray[k] == g_productDatas[i].DeliverableID)
                    {
                        @g_productDatas[i].Display = pi.Display;
                        break;
                    }
                }
                if (g_productDatas[i].Display !is null) break;
            }
        }
    }

    g_dataLoaded = true;
    return true;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Helper to parse int from string
int parseInt(string s)
{
    int result = 0;
    int sign = 1;
    uint i = 0;
    if (s.length() > 0 && s.substr(0, 1) == "-")
    {
        sign = -1;
        i = 1;
    }
    for (; i < s.length(); i++)
    {
        string c = s.substr(i, 1);
        if (c >= "0" && c <= "9")
            result = result * 10 + (c[0] - 48);
    }
    return result * sign;
}
