/*
 * ASDB2Schema.cpp
 * 
 * Implementation of dynamic DB2 schema system for AngelScript.
 */

#include "ASDB2Schema.h"
#include "Log.h"
#include "DB2Meta.h"

#ifdef ANGELSCRIPT_INTEGRATION

namespace AngelScript
{
    // ========================================================================
    // ASDB2Schema Implementation
    // ========================================================================

    ASDB2Schema::ASDB2Schema(const std::string& name)
        : _name(name), _recordSize(0), _finalized(false)
    {
    }

    ASDB2Schema::~ASDB2Schema() = default;

    void ASDB2Schema::AddField(const std::string& name, DB2FieldType type, uint32 arraySize, bool isSigned)
    {
        if (_finalized)
        {
            TC_LOG_ERROR("angelscript.db2", "Cannot add field '{}' to finalized schema '{}'", name, _name);
            return;
        }

        if (_fieldNameToIndex.find(name) != _fieldNameToIndex.end())
        {
            TC_LOG_ERROR("angelscript.db2", "Field '{}' already exists in schema '{}'", name, _name);
            return;
        }

        uint32 index = static_cast<uint32>(_fields.size());
        _fields.emplace_back(name, type, arraySize, isSigned);
        _fieldNameToIndex[name] = index;
    }

    void ASDB2Schema::AddInt8(const std::string& name, uint32 count) { AddField(name, DB2FieldType::INT8, count); }
    void ASDB2Schema::AddUInt8(const std::string& name, uint32 count) { AddField(name, DB2FieldType::UINT8, count); }
    void ASDB2Schema::AddInt16(const std::string& name, uint32 count) { AddField(name, DB2FieldType::INT16, count); }
    void ASDB2Schema::AddUInt16(const std::string& name, uint32 count) { AddField(name, DB2FieldType::UINT16, count); }
    void ASDB2Schema::AddInt32(const std::string& name, uint32 count) { AddField(name, DB2FieldType::INT32, count); }
    void ASDB2Schema::AddUInt32(const std::string& name, uint32 count) { AddField(name, DB2FieldType::UINT32, count); }
    void ASDB2Schema::AddInt64(const std::string& name, uint32 count) { AddField(name, DB2FieldType::INT64, count); }
    void ASDB2Schema::AddUInt64(const std::string& name, uint32 count) { AddField(name, DB2FieldType::UINT64, count); }
    void ASDB2Schema::AddFloat(const std::string& name, uint32 count) { AddField(name, DB2FieldType::FLOAT, count); }
    void ASDB2Schema::AddDouble(const std::string& name, uint32 count) { AddField(name, DB2FieldType::DOUBLE, count); }
    void ASDB2Schema::AddString(const std::string& name, uint32 count) { AddField(name, DB2FieldType::STRING, count); }
    void ASDB2Schema::AddBool(const std::string& name, uint32 count) { AddField(name, DB2FieldType::BOOL, count); }

    const ASDB2FieldDef* ASDB2Schema::GetField(uint32 index) const
    {
        if (index >= _fields.size())
            return nullptr;
        return &_fields[index];
    }

    const ASDB2FieldDef* ASDB2Schema::GetField(const std::string& name) const
    {
        auto it = _fieldNameToIndex.find(name);
        if (it == _fieldNameToIndex.end())
            return nullptr;
        return &_fields[it->second];
    }

    int32 ASDB2Schema::GetFieldIndex(const std::string& name) const
    {
        auto it = _fieldNameToIndex.find(name);
        if (it == _fieldNameToIndex.end())
            return -1;
        return static_cast<int32>(it->second);
    }

    uint32 ASDB2Schema::GetFieldTypeSize(DB2FieldType type)
    {
        switch (type)
        {
            case DB2FieldType::INT8:
            case DB2FieldType::UINT8:
            case DB2FieldType::BOOL:
                return 1;
            case DB2FieldType::INT16:
            case DB2FieldType::UINT16:
                return 2;
            case DB2FieldType::INT32:
            case DB2FieldType::UINT32:
            case DB2FieldType::FLOAT:
            case DB2FieldType::STRING:
            case DB2FieldType::STRING_REF:
                return 4;
            case DB2FieldType::INT64:
            case DB2FieldType::UINT64:
            case DB2FieldType::DOUBLE:
                return 8;
            case DB2FieldType::LOCALIZED_STRING:
                return 4; // Pointer to localized data
            default:
                return 4;
        }
    }

    DBCFormer ASDB2Schema::ToTCFieldType(DB2FieldType type)
    {
        switch (type)
        {
            case DB2FieldType::INT8:
            case DB2FieldType::UINT8:
            case DB2FieldType::BOOL:
                return FT_BYTE;
            case DB2FieldType::INT16:
            case DB2FieldType::UINT16:
                return FT_SHORT;
            case DB2FieldType::INT32:
            case DB2FieldType::UINT32:
                return FT_INT;
            case DB2FieldType::INT64:
            case DB2FieldType::UINT64:
                return FT_LONG;
            case DB2FieldType::FLOAT:
                return FT_FLOAT;
            case DB2FieldType::STRING:
            case DB2FieldType::STRING_REF:
                return FT_STRING;
            case DB2FieldType::LOCALIZED_STRING:
                return FT_STRING_NOT_LOCALIZED;
            default:
                return FT_INT;
        }
    }

    void ASDB2Schema::Finalize()
    {
        if (_finalized)
            return;

        // Compute offsets and sizes
        uint32 currentOffset = 0;
        for (auto& field : _fields)
        {
            uint32 elementSize = GetFieldTypeSize(field.Type);
            field.Size = elementSize * field.ArraySize;
            field.Offset = currentOffset;
            currentOffset += field.Size;
        }

        _recordSize = currentOffset;
        _finalized = true;

        TC_LOG_INFO("angelscript.db2", "Schema '{}' finalized: {} fields, {} bytes/record", 
            _name, _fields.size(), _recordSize);
    }

    // ========================================================================
    // ASDB2SchemaRegistry Implementation
    // ========================================================================

    ASDB2SchemaRegistry* ASDB2SchemaRegistry::instance()
    {
        static ASDB2SchemaRegistry instance;
        return &instance;
    }

    void ASDB2SchemaRegistry::RegisterSchema(std::shared_ptr<ASDB2Schema> schema)
    {
        if (!schema)
            return;

        const std::string& name = schema->GetName();
        _schemas[name] = schema;
        
        TC_LOG_INFO("angelscript.db2", "Registered DB2 schema: {}", name);
    }

    ASDB2Schema* ASDB2SchemaRegistry::GetSchema(const std::string& name) const
    {
        auto it = _schemas.find(name);
        if (it == _schemas.end())
            return nullptr;
        return it->second.get();
    }

    bool ASDB2SchemaRegistry::HasSchema(const std::string& name) const
    {
        return _schemas.find(name) != _schemas.end();
    }

    void ASDB2SchemaRegistry::RemoveSchema(const std::string& name)
    {
        _schemas.erase(name);
    }

    std::vector<std::string> ASDB2SchemaRegistry::GetSchemaNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : _schemas)
            names.push_back(pair.first);
        return names;
    }

} // namespace AngelScript

#endif // ANGELSCRIPT_INTEGRATION
