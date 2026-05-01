#ifndef ASGENERICDB2LOADER_H
#define ASGENERICDB2LOADER_H

#ifdef ANGELSCRIPT_INTEGRATION

#include "Define.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Generic DB2 file loader for reading any DB2 file at runtime
// No compile-time structures needed - completely dynamic

namespace AngelScript
{
    // DB2 file header (common to most DB2 versions)
    #pragma pack(push, 1)
    struct DB2FileHeader
    {
        uint32 Signature;
        uint32 Version;
        char   Schema[128];
        uint32 RecordCount;
        uint32 FieldCount;
        uint32 RecordSize;
        uint32 StringTableSize;
        uint32 TableHash;
        uint32 LayoutHash;
        uint32 MinId;
        uint32 MaxId;
        uint32 Locale;
        uint16 Flags;
        int16  IndexField;
        uint32 TotalFieldCount;
        uint32 PackedDataOffset;
        uint32 ParentLookupCount;
        uint32 ColumnMetaSize;
        uint32 CommonDataSize;
        uint32 PalletDataSize;
        uint32 SectionCount;
    };

    struct DB2SectionHeader
    {
        uint64 TactId;
        uint32 FileOffset;
        uint32 RecordCount;
        uint32 StringTableSize;
        uint32 CatalogDataOffset;
        uint32 IdTableSize;
        uint32 ParentLookupDataSize;
        uint32 CatalogDataCount;
        uint32 CopyTableCount;
    };
    #pragma pack(pop)

    // Field info for dynamic parsing
    struct DB2FieldInfo
    {
        uint32 Offset;          // Offset in record
        uint32 Size;            // Size in bytes (4, 2, 1, or variable)
        uint32 Type;            // 0=int32, 1=uint32, 2=float, 3=string, 4=int16, 5=uint16, 6=int8, 7=uint8
        std::string Name;       // Field name (if known from meta)
    };

    // Single record as dynamic data
    class DB2DynamicRecord
    {
    public:
        DB2DynamicRecord() : _id(0), _recordSize(0) {}
        
        void SetId(uint32 id) { _id = id; }
        uint32 GetId() const { return _id; }
        
        void SetRawData(const uint8* data, uint32 size);
        
        // Getters for different field types
        int32 GetInt32(uint32 fieldIndex) const;
        uint32 GetUInt32(uint32 fieldIndex) const;
        float GetFloat(uint32 fieldIndex) const;
        std::string GetString(uint32 fieldIndex, const char* stringPool, uint32 poolSize) const;
        int16 GetInt16(uint32 fieldIndex) const;
        uint16 GetUInt16(uint32 fieldIndex) const;
        int8 GetInt8(uint32 fieldIndex) const;
        uint8 GetUInt8(uint32 fieldIndex) const;
        uint64 GetUInt64(uint32 fieldIndex) const;
        
        // Raw access
        const uint8* GetRawData() const { return _data.data(); }
        uint32 GetSize() const { return _recordSize; }
        
    private:
        uint32 _id;
        uint32 _recordSize;
        std::vector<uint8> _data;
    };

    // Generic DB2 file loader
    class GenericDB2Loader
    {
    public:
        GenericDB2Loader();
        ~GenericDB2Loader();
        
        // Load a DB2 file from disk
        bool LoadFile(const std::string& filepath);
        bool LoadFromData(const std::vector<uint8>& data);
        
        // Query methods
        bool HasRecord(uint32 id) const;
        const DB2DynamicRecord* GetRecord(uint32 id) const;
        const DB2DynamicRecord* GetRecordByIndex(uint32 index) const;
        
        // Iterator
        uint32 GetRecordCount() const { return static_cast<uint32>(_records.size()); }
        std::vector<uint32> GetAllIds() const;
        
        // Find records by field value
        std::vector<uint32> FindRecords(uint32 fieldIndex, uint32 value) const;
        
        // Metadata
        uint32 GetFieldCount() const { return _header.FieldCount; }
        uint32 GetRecordSize() const { return _header.RecordSize; }
        std::string GetTableHashString() const;
        
        // Field info
        void SetFieldInfo(uint32 index, const DB2FieldInfo& info);
        const DB2FieldInfo* GetFieldInfo(uint32 index) const;
        
        // Get available tables (static list of known DB2 files)
        static std::vector<std::string> GetKnownDB2Files();
        
    private:
        bool ParseWDB2(const uint8* data, size_t size);
        bool ParseWDB5(const uint8* data, size_t size);
        bool ParseWDB6(const uint8* data, size_t size);
        bool ParseWDC1(const uint8* data, size_t size);
        bool ParseWDC2(const uint8* data, size_t size);
        bool ParseWDC5(const uint8* data, size_t size);
        
        DB2FileHeader _header;
        std::vector<DB2DynamicRecord> _records;
        std::map<uint32, uint32> _idToIndex;  // Map ID -> record index
        std::vector<uint8> _stringPool;
        std::vector<DB2FieldInfo> _fieldInfo;
        
        // Raw file data
        std::vector<uint8> _fileData;
    };

    // Manager for loaded DB2 files
    class DB2FileManager
    {
    public:
        static DB2FileManager* instance();
        
        // Load a DB2 file (returns handle/id)
        uint32 LoadDB2File(const std::string& filename);
        
        // Get loaded file by handle
        GenericDB2Loader* GetLoader(uint32 handle);
        
        // Unload file
        void UnloadFile(uint32 handle);
        void UnloadAll();
        
        // List loaded files
        std::vector<std::pair<uint32, std::string>> GetLoadedFiles() const;
        
    private:
        DB2FileManager() = default;
        ~DB2FileManager() = default;
        
        std::map<uint32, std::unique_ptr<GenericDB2Loader>> _loaders;
        std::map<uint32, std::string> _fileNames;
        uint32 _nextHandle = 1;
    };

} // namespace AngelScript

#define sASDB2FileMgr AngelScript::DB2FileManager::instance()

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASGENERICDB2LOADER_H
