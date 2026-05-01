/*
 * ASDB2Schema.h
 * 
 * Dynamic DB2 schema definition for AngelScript.
 * Allows runtime definition of DB2 structures without compile-time C++ structs.
 * 
 * Usage in AngelScript:
 *   DB2Schema@ mySchema = DB2Schema("MyCustomDB2");
 *   mySchema.AddField("ID", DB2FIELD_UINT32);
 *   mySchema.AddField("Name", DB2FIELD_STRING);
 *   mySchema.AddField("Value", DB2FIELD_FLOAT);
 *   
 *   uint32 db2Handle = LoadDB2WithSchema("path/to/file.db2", mySchema);
 */

#ifndef ASDB2SCHEMA_H
#define ASDB2SCHEMA_H

#ifdef ANGELSCRIPT_INTEGRATION

#include "Define.h"
#include "DB2Meta.h"
#include <string>

#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace AngelScript
{
    // Field types matching TC's DB2 field types
    enum class DB2FieldType : uint8
    {
        INT8 = 0,       // Signed byte
        UINT8,          // Unsigned byte
        INT16,          // Signed short
        UINT16,         // Unsigned short
        INT32,          // Signed int
        UINT32,         // Unsigned int
        INT64,          // Signed long long
        UINT64,         // Unsigned long long
        FLOAT,          // Float
        DOUBLE,         // Double
        STRING,         // String (offset into string pool)
        STRING_REF,     // String reference
        LOCALIZED_STRING, // Localized string (multiple locales)
        BOOL,           // Boolean (stored as byte)
    };

    // Single field definition in a schema
    struct ASDB2FieldDef
    {
        std::string Name;
        DB2FieldType Type;
        uint32 ArraySize;       // For array fields (default 1)
        bool IsSigned;          // For integer types
        uint32 Offset;          // Computed offset in record
        uint32 Size;            // Computed size in bytes
        
        ASDB2FieldDef() : Type(DB2FieldType::INT32), ArraySize(1), IsSigned(true), Offset(0), Size(4) {}
        ASDB2FieldDef(const std::string& name, DB2FieldType type, uint32 arraySize = 1, bool isSigned = true)
            : Name(name), Type(type), ArraySize(arraySize), IsSigned(isSigned), Offset(0), Size(0) {}
    };

    // Dynamic schema for a DB2 file
    class ASDB2Schema
    {
    public:
        ASDB2Schema(const std::string& name);
        ~ASDB2Schema();

        // Schema building
        void AddField(const std::string& name, DB2FieldType type, uint32 arraySize = 1, bool isSigned = true);
        void AddInt8(const std::string& name, uint32 count = 1);
        void AddUInt8(const std::string& name, uint32 count = 1);
        void AddInt16(const std::string& name, uint32 count = 1);
        void AddUInt16(const std::string& name, uint32 count = 1);
        void AddInt32(const std::string& name, uint32 count = 1);
        void AddUInt32(const std::string& name, uint32 count = 1);
        void AddInt64(const std::string& name, uint32 count = 1);
        void AddUInt64(const std::string& name, uint32 count = 1);
        void AddFloat(const std::string& name, uint32 count = 1);
        void AddDouble(const std::string& name, uint32 count = 1);
        void AddString(const std::string& name, uint32 count = 1);
        void AddBool(const std::string& name, uint32 count = 1);

        // Schema info
        const std::string& GetName() const { return _name; }
        uint32 GetFieldCount() const { return static_cast<uint32>(_fields.size()); }
        uint32 GetRecordSize() const { return _recordSize; }
        const ASDB2FieldDef* GetField(uint32 index) const;
        const ASDB2FieldDef* GetField(const std::string& name) const;
        int32 GetFieldIndex(const std::string& name) const;

        // Compute offsets and finalize schema
        void Finalize();
        bool IsFinalized() const { return _finalized; }

        // Type helpers
        static uint32 GetFieldTypeSize(DB2FieldType type);
        static DBCFormer ToTCFieldType(DB2FieldType type);

    private:
        std::string _name;
        std::vector<ASDB2FieldDef> _fields;
        std::unordered_map<std::string, uint32> _fieldNameToIndex;
        uint32 _recordSize;
        bool _finalized;
    };

    // Registry of all custom schemas
    class ASDB2SchemaRegistry
    {
    public:
        static ASDB2SchemaRegistry* instance();

        // Register a schema
        void RegisterSchema(std::shared_ptr<ASDB2Schema> schema);
        
        // Get schema by name
        ASDB2Schema* GetSchema(const std::string& name) const;
        
        // Check if schema exists
        bool HasSchema(const std::string& name) const;
        
        // Remove schema
        void RemoveSchema(const std::string& name);
        
        // List all schemas
        std::vector<std::string> GetSchemaNames() const;

    private:
        ASDB2SchemaRegistry() = default;
        ~ASDB2SchemaRegistry() = default;

        std::unordered_map<std::string, std::shared_ptr<ASDB2Schema>> _schemas;
    };

} // namespace AngelScript

#define sASDB2SchemaRegistry AngelScript::ASDB2SchemaRegistry::instance()

#endif // ANGELSCRIPT_INTEGRATION
#endif // ASDB2SCHEMA_H
