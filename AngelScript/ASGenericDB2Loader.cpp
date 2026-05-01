#include "ASGenericDB2Loader.h"
#include "Log.h"
#include <fstream>
#include <cstring>

#ifdef ANGELSCRIPT_INTEGRATION

namespace AngelScript
{
    // DB2 Signatures
    constexpr uint32 WDB2_SIGNATURE = 0x32424457; // 'WDB2'
    constexpr uint32 WDB5_SIGNATURE = 0x35424457; // 'WDB5'
    constexpr uint32 WDB6_SIGNATURE = 0x36424457; // 'WDB6'
    constexpr uint32 WDC1_SIGNATURE = 0x31434457; // 'WDC1'
    constexpr uint32 WDC2_SIGNATURE = 0x32434457; // 'WDC2'
    constexpr uint32 WDC3_SIGNATURE = 0x33434457; // 'WDC3'
    constexpr uint32 WDC4_SIGNATURE = 0x34434457; // 'WDC4'
    constexpr uint32 WDC5_SIGNATURE = 0x35434457; // 'WDC5'

    // DB2DynamicRecord implementation
    void DB2DynamicRecord::SetRawData(const uint8* data, uint32 size)
    {
        _data.resize(size);
        if (size > 0 && data)
            memcpy(_data.data(), data, size);
        _recordSize = size;
    }

    int32 DB2DynamicRecord::GetInt32(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(int32);
        if (offset + sizeof(int32) > _recordSize)
            return 0;
        return *reinterpret_cast<const int32*>(_data.data() + offset);
    }

    uint32 DB2DynamicRecord::GetUInt32(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(uint32);
        if (offset + sizeof(uint32) > _recordSize)
            return 0;
        return *reinterpret_cast<const uint32*>(_data.data() + offset);
    }

    float DB2DynamicRecord::GetFloat(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(float);
        if (offset + sizeof(float) > _recordSize)
            return 0.0f;
        return *reinterpret_cast<const float*>(_data.data() + offset);
    }

    std::string DB2DynamicRecord::GetString(uint32 fieldIndex, const char* stringPool, uint32 poolSize) const
    {
        // String is stored as offset into string pool
        uint32 stringOffset = GetUInt32(fieldIndex);
        if (stringOffset >= poolSize || !stringPool)
            return "";
        return std::string(stringPool + stringOffset);
    }

    int16 DB2DynamicRecord::GetInt16(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(int32); // Still 4-byte aligned
        if (offset + sizeof(int16) > _recordSize)
            return 0;
        return *reinterpret_cast<const int16*>(_data.data() + offset);
    }

    uint16 DB2DynamicRecord::GetUInt16(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(uint32);
        if (offset + sizeof(uint16) > _recordSize)
            return 0;
        return *reinterpret_cast<const uint16*>(_data.data() + offset);
    }

    int8 DB2DynamicRecord::GetInt8(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(uint32);
        if (offset + sizeof(int8) > _recordSize)
            return 0;
        return *reinterpret_cast<const int8*>(_data.data() + offset);
    }

    uint8 DB2DynamicRecord::GetUInt8(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(uint32);
        if (offset + sizeof(uint8) > _recordSize)
            return 0;
        return *reinterpret_cast<const uint8*>(_data.data() + offset);
    }

    uint64 DB2DynamicRecord::GetUInt64(uint32 fieldIndex) const
    {
        uint32 offset = fieldIndex * sizeof(uint32);
        if (offset + sizeof(uint64) > _recordSize)
            return 0;
        return *reinterpret_cast<const uint64*>(_data.data() + offset);
    }

    // GenericDB2Loader implementation
    GenericDB2Loader::GenericDB2Loader()
    {
        memset(&_header, 0, sizeof(_header));
    }

    GenericDB2Loader::~GenericDB2Loader() = default;

    bool GenericDB2Loader::LoadFile(const std::string& filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            TC_LOG_ERROR("angelscript.db2", "Cannot open DB2 file: {}", filepath);
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        _fileData.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(_fileData.data()), size))
        {
            TC_LOG_ERROR("angelscript.db2", "Failed to read DB2 file: {}", filepath);
            return false;
        }

        file.close();

        return LoadFromData(_fileData);
    }

    bool GenericDB2Loader::LoadFromData(const std::vector<uint8>& data)
    {
        if (data.size() < sizeof(DB2FileHeader))
        {
            TC_LOG_ERROR("angelscript.db2", "DB2 data too small for header");
            return false;
        }

        memcpy(&_header, data.data(), sizeof(DB2FileHeader));

        // Check signature and parse accordingly
        switch (_header.Signature)
        {
            case WDB2_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDB2 format ({} records)", _header.RecordCount);
                return ParseWDB2(data.data(), data.size());
                
            case WDB5_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDB5 format ({} records)", _header.RecordCount);
                return ParseWDB5(data.data(), data.size());
                
            case WDB6_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDB6 format ({} records)", _header.RecordCount);
                return ParseWDB6(data.data(), data.size());
                
            case WDC1_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDC1 format ({} records)", _header.RecordCount);
                return ParseWDC1(data.data(), data.size());
                
            case WDC2_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDC2 format ({} records)", _header.RecordCount);
                return ParseWDC2(data.data(), data.size());

            case WDC3_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDC3 format ({} records)", _header.RecordCount);
                return ParseWDB2(data.data(), data.size());

            case WDC4_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDC4 format ({} records)", _header.RecordCount);
                return ParseWDB2(data.data(), data.size());

            case WDC5_SIGNATURE:
                TC_LOG_INFO("angelscript.db2", "Parsing WDC5 format ({} records)", _header.RecordCount);
                return ParseWDC5(data.data(), data.size());
                
            default:
                TC_LOG_ERROR("angelscript.db2", "Unknown DB2 signature: 0x{:08X}", _header.Signature);
                return false;
        }
    }

    bool GenericDB2Loader::ParseWDB2(const uint8* data, size_t size)
    {
        // WDB2: Simple format with ID at end of record
        // Layout: Header | Records | String Table | Copy Table | IDs
        
        size_t headerSize = sizeof(DB2FileHeader);
        size_t recordsSize = static_cast<size_t>(_header.RecordCount) * _header.RecordSize;
        size_t stringTableOffset = headerSize + recordsSize;
        
        // Read string table
        if (_header.StringTableSize > 0 && stringTableOffset + _header.StringTableSize <= size)
        {
            _stringPool.resize(_header.StringTableSize);
            memcpy(_stringPool.data(), data + stringTableOffset, _header.StringTableSize);
        }

        // Parse records
        _records.resize(_header.RecordCount);
        for (uint32 i = 0; i < _header.RecordCount; ++i)
        {
            const uint8* recordData = data + headerSize + (i * _header.RecordSize);
            
            _records[i].SetRawData(recordData, _header.RecordSize);
            
            // In WDB2, ID is at the end of record (last 4 bytes typically)
            uint32 id = *reinterpret_cast<const uint32*>(recordData + _header.RecordSize - 4);
            _records[i].SetId(id);
            _idToIndex[id] = i;
        }

        TC_LOG_INFO("angelscript.db2", "Loaded {} WDB2 records", _records.size());
        return true;
    }

    bool GenericDB2Loader::ParseWDB5(const uint8* data, size_t size)
    {
        // WDB5: More complex with optional ID field and common data
        // For now, simplified parsing
        return ParseWDB2(data, size); // Similar structure for basic reading
    }

    bool GenericDB2Loader::ParseWDB6(const uint8* data, size_t size)
    {
        // WDB6: Added section structure
        // Simplified parsing for now
        return ParseWDB2(data, size);
    }

    bool GenericDB2Loader::ParseWDC1(const uint8* data, size_t size)
    {
        // WDC1: Major format change with compressed data
        TC_LOG_WARN("angelscript.db2", "WDC1 format not fully implemented, attempting basic parse");
        return ParseWDB2(data, size);
    }

    bool GenericDB2Loader::ParseWDC2(const uint8* data, size_t size)
    {
        // WDC2: Newest format
        TC_LOG_WARN("angelscript.db2", "WDC2 format not fully implemented, attempting basic parse");
        return ParseWDB2(data, size);
    }

    bool GenericDB2Loader::ParseWDC5(const uint8* data, size_t size)
    {
        uint32 headerSize = sizeof(DB2FileHeader);
        if (size < headerSize) return false;

        const DB2FileHeader* header = reinterpret_cast<const DB2FileHeader*>(data);
        uint32 sectionCount = header->SectionCount;
        if (sectionCount == 0) return false;

        // The first section header follows the file header
        const DB2SectionHeader* section = reinterpret_cast<const DB2SectionHeader*>(data + headerSize);
        
        uint32 totalRecords = section->RecordCount;
        uint32 recordSize = header->RecordSize;
        uint32 dataOffset = section->FileOffset;

        if (dataOffset + (totalRecords * recordSize) > size)
        {
            TC_LOG_ERROR("angelscript.db2", "WDC5 records out of bounds: offset={} count={} size={}", dataOffset, totalRecords, size);
            return false;
        }

        // Parse IDs (optional, check IdTableSize)
        const uint32* ids = nullptr;
        if (section->IdTableSize > 0)
        {
             size_t idTableOffset = dataOffset + (totalRecords * recordSize);
             if (idTableOffset + section->IdTableSize <= size)
                 ids = reinterpret_cast<const uint32*>(data + idTableOffset);
        }

        _records.clear();
        _records.resize(totalRecords);
        for (uint32 i = 0; i < totalRecords; ++i)
        {
            const uint8* recordData = data + dataOffset + (i * recordSize);
            _records[i].SetRawData(recordData, recordSize);
            
            uint32 id = (ids) ? ids[i] : i; // Fallback to index if no ID table
            _records[i].SetId(id);
            _idToIndex[id] = i;
        }

        _header = *header;
        _header.RecordCount = totalRecords; // Sync with section record count

        TC_LOG_INFO("angelscript.db2", "Loaded {} WDC5 records", _records.size());
        return true;
    }

    bool GenericDB2Loader::HasRecord(uint32 id) const
    {
        return _idToIndex.find(id) != _idToIndex.end();
    }

    const DB2DynamicRecord* GenericDB2Loader::GetRecord(uint32 id) const
    {
        auto it = _idToIndex.find(id);
        if (it == _idToIndex.end())
            return nullptr;
        return &_records[it->second];
    }

    const DB2DynamicRecord* GenericDB2Loader::GetRecordByIndex(uint32 index) const
    {
        if (index >= _records.size())
            return nullptr;
        return &_records[index];
    }

    std::vector<uint32> GenericDB2Loader::GetAllIds() const
    {
        std::vector<uint32> ids;
        ids.reserve(_idToIndex.size());
        for (const auto& pair : _idToIndex)
            ids.push_back(pair.first);
        return ids;
    }

    std::vector<uint32> GenericDB2Loader::FindRecords(uint32 fieldIndex, uint32 value) const
    {
        std::vector<uint32> results;
        for (const auto& pair : _idToIndex)
        {
            const DB2DynamicRecord& record = _records[pair.second];
            if (record.GetUInt32(fieldIndex) == value)
                results.push_back(pair.first);
        }
        return results;
    }

    std::string GenericDB2Loader::GetTableHashString() const
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "0x%08X", _header.TableHash);
        return std::string(buf);
    }

    void GenericDB2Loader::SetFieldInfo(uint32 index, const DB2FieldInfo& info)
    {
        if (index >= _fieldInfo.size())
            _fieldInfo.resize(index + 1);
        _fieldInfo[index] = info;
    }

    const DB2FieldInfo* GenericDB2Loader::GetFieldInfo(uint32 index) const
    {
        if (index >= _fieldInfo.size())
            return nullptr;
        return &_fieldInfo[index];
    }

    std::vector<std::string> GenericDB2Loader::GetKnownDB2Files()
    {
        return {
            "Achievement.db2",
            "AreaTable.db2",
            "CharTitles.db2",
            "Creature.db2",
            "CreatureDisplayInfo.db2",
            "CurrencyTypes.db2",
            "GameObjectDisplayInfo.db2",
            "Item.db2",
            "ItemBonus.db2",
            "ItemXItemEffect.db2",
            "Map.db2",
            "QuestV2.db2",
            "SkillLine.db2",
            "Spell.db2",
            "SpellEffect.db2",
            "SpellName.db2",
            "Talent.db2",
            "WorldState.db2"
        };
    }

    // DB2FileManager implementation
    DB2FileManager* DB2FileManager::instance()
    {
        static DB2FileManager instance;
        return &instance;
    }

    uint32 DB2FileManager::LoadDB2File(const std::string& filename)
    {
        // Check if already loaded
        for (const auto& pair : _fileNames)
        {
            if (pair.second == filename)
                return pair.first; // Return existing handle
        }

        auto loader = std::make_unique<GenericDB2Loader>();
        if (!loader->LoadFile(filename))
        {
            TC_LOG_ERROR("angelscript.db2", "Failed to load DB2 file: {}", filename);
            return 0;
        }

        uint32 handle = _nextHandle++;
        _loaders[handle] = std::move(loader);
        _fileNames[handle] = filename;

        TC_LOG_INFO("angelscript.db2", "Loaded DB2 file '{}' with handle {}", filename, handle);
        return handle;
    }

    GenericDB2Loader* DB2FileManager::GetLoader(uint32 handle)
    {
        auto it = _loaders.find(handle);
        if (it == _loaders.end())
            return nullptr;
        return it->second.get();
    }

    void DB2FileManager::UnloadFile(uint32 handle)
    {
        _loaders.erase(handle);
        _fileNames.erase(handle);
        TC_LOG_INFO("angelscript.db2", "Unloaded DB2 file handle {}", handle);
    }

    void DB2FileManager::UnloadAll()
    {
        _loaders.clear();
        _fileNames.clear();
        TC_LOG_INFO("angelscript.db2", "Unloaded all DB2 files");
    }

    std::vector<std::pair<uint32, std::string>> DB2FileManager::GetLoadedFiles() const
    {
        std::vector<std::pair<uint32, std::string>> result;
        for (const auto& pair : _fileNames)
            result.emplace_back(pair.first, pair.second);
        return result;
    }

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
