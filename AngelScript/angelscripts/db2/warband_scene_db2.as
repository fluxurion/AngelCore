/*
 * warband_scene_db2.as
 * DB2 loader and accessor for WarbandScene.db2
 */

#include "../includes/ScriptFramework.as"

const string WARBAND_SCENE_STORAGE = "WarbandScene";
array<uint32> g_cachedWarbandSceneIds;
bool g_warbandSceneDb2Loaded = false;

void LoadWarbandSceneDB2()
{
    if (g_warbandSceneDb2Loaded) return;

    string db2Path = GetDB2Path(WARBAND_SCENE_STORAGE);
    Print("[DB2Loader] Loading " + WARBAND_SCENE_STORAGE + " from " + db2Path);

    DB2Schema@ schema = CreateDB2Schema(WARBAND_SCENE_STORAGE);
    if (schema is null) return;

    schema.AddString("Name_lang");        // 0
    schema.AddString("Description_lang"); // 1
    schema.AddFloat("Position_0");        // 2
    schema.AddFloat("Position_1");        // 3
    schema.AddFloat("Position_2");        // 4
    schema.AddFloat("LookAt_0");          // 5
    schema.AddFloat("LookAt_1");          // 6
    schema.AddFloat("LookAt_2");          // 7
    schema.AddUInt32("ID");               // 8
    schema.AddInt32("MapID");             // 9
    schema.AddFloat("Fov");               // 10
    schema.AddInt32("TimeOfDay");         // 11
    schema.AddInt32("Flags");             // 12
    schema.AddInt32("SoundAmbienceID");   // 13
    schema.AddInt8("Quality");            // 14
    schema.AddInt32("TextureKit");        // 15
    schema.AddInt32("DefaultScenePriority"); // 16
    schema.Finalize();

    uint32 handle = LoadDB2Storage(WARBAND_SCENE_STORAGE, db2Path, schema);
    if (handle == 0)
    {
        // Try fallback without enUS
        string dataDir = GetConfigString("DataDir", ".");
        db2Path = dataDir + "/dbc/" + WARBAND_SCENE_STORAGE + ".db2";
        Print("[DB2Loader] Fallback loading from " + db2Path);
        handle = LoadDB2Storage(WARBAND_SCENE_STORAGE, db2Path, schema);
    }

    if (handle != 0)
    {
        DB2Storage@ store = GetDB2Storage(WARBAND_SCENE_STORAGE);
        if (store !is null)
        {
            g_cachedWarbandSceneIds = store.GetAllIds();
            g_warbandSceneDb2Loaded = true;
            Print("[DB2Loader] Successfully loaded " + g_cachedWarbandSceneIds.length() + " warband scenes");
        }
    }
    else
    {
        Print("[DB2Loader] FAILED to load " + WARBAND_SCENE_STORAGE);
    }
}

void EnsureWarbandSceneDB2Loaded()
{
    if (!g_warbandSceneDb2Loaded)
        LoadWarbandSceneDB2();
}

array<uint32> GetAllWarbandSceneIds()
{
    EnsureWarbandSceneDB2Loaded();
    return g_cachedWarbandSceneIds;
}

// Hook to load on server startup
void OnWarbandSceneWorldStartup()
{
    LoadWarbandSceneDB2();
}

void RegisterWarbandSceneDB2()
{
    RegisterWorldScript(WORLD_ON_STARTUP, @OnWarbandSceneWorldStartup);
}
