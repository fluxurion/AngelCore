/*
 * AngelScript Packet API
 * Packet read/write for handling unimplemented opcodes and custom packets
 */

#ifndef ANGELSCRIPT_INTEGRATION
    #error "ANGELSCRIPT_INTEGRATION macro must be defined"
#endif

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#pragma push_macro("IN")
#pragma push_macro("OUT")
#pragma push_macro("OPTIONAL")
#undef IN
#undef OUT
#undef OPTIONAL

#include <angelscript.h>

#pragma pop_macro("OPTIONAL")
#pragma pop_macro("OUT")
#pragma pop_macro("IN")

#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "Opcodes.h"
#include "Log.h"
#include "../ASPacketData.h"
#include "../Hooks/ASPacketHooks.h"
#include "ByteBuffer.h"
#include "ObjectGuid.h"
#include "World.h"

namespace AngelScript
{
    // ---- WorldSession wrappers ----
    static uint32  Session_GetAccountId(WorldSession* s)          { return s ? s->GetAccountId() : 0; }
    static uint32  Session_GetBattlenetAccountId(WorldSession* s)  { return s ? s->GetBattlenetAccountId() : 0; }
    static std::string Session_GetAccountName(WorldSession* s)     { return s ? s->GetAccountName() : ""; }
    static Player* Session_GetPlayer(WorldSession* s)              { return s ? s->GetPlayer() : nullptr; }
    static void    Session_SendPacket(WorldSession* s, PacketData* pd)
    {
        if (!s || !pd) return;
        WorldPacket pkt(static_cast<OpcodeServer>(pd->opcode));
        if (!pd->data.empty())
            pkt.append(pd->data.data(), pd->data.size());
        s->SendPacket(&pkt, true);
    }

    // ---- OpcodeHandler registration (script -> C++ bridge) ----
    static void RegisterOpcodeHandler(uint32 opcode, asIScriptFunction* func, bool blockOriginal)
    {
        if (!func) return;
        ASPacketHooks::instance()->RegisterOpcodeHandler(opcode, func, blockOriginal);
    }
    static void UnregisterOpcodeHandler(uint32 opcode)
    {
        ASPacketHooks::instance()->UnregisterOpcodeHandler(opcode);
    }

    // ---- PacketData wrappers (from ASPacketData.h) ----

    static uint8 PD_ReadUInt8(PacketData* pd) { return pd ? pd->ReadUInt8() : 0; }
    static uint16 PD_ReadUInt16(PacketData* pd) { return pd ? pd->ReadUInt16() : 0; }
    static uint32 PD_ReadUInt32(PacketData* pd) { return pd ? pd->ReadUInt32() : 0; }
    static uint64 PD_ReadUInt64(PacketData* pd) { return pd ? pd->ReadUInt64() : 0; }
    static int8 PD_ReadInt8(PacketData* pd) { return pd ? pd->ReadInt8() : 0; }
    static int16 PD_ReadInt16(PacketData* pd) { return pd ? pd->ReadInt16() : 0; }
    static int32 PD_ReadInt32(PacketData* pd) { return pd ? pd->ReadInt32() : 0; }
    static int64 PD_ReadInt64(PacketData* pd) { return pd ? pd->ReadInt64() : 0; }
    static float PD_ReadFloat(PacketData* pd) { return pd ? pd->ReadFloat() : 0.f; }
    static double PD_ReadDouble(PacketData* pd) { return pd ? pd->ReadDouble() : 0.0; }
    static std::string PD_ReadString(PacketData* pd) { return pd ? pd->ReadString() : ""; }
    static std::string PD_ReadCString(PacketData* pd) { return pd ? pd->ReadCString() : ""; }
    static void PD_WriteUInt32At(PacketData* pd, uint32 offset, uint32 value)
    {
        if (!pd || offset + 4 > pd->data.size()) return;
        pd->data[offset]     = static_cast<uint8>(value);
        pd->data[offset + 1] = static_cast<uint8>(value >> 8);
        pd->data[offset + 2] = static_cast<uint8>(value >> 16);
        pd->data[offset + 3] = static_cast<uint8>(value >> 24);
    }
    static uint32 PD_ReadUInt32At(PacketData* pd, uint32 offset)
    {
        if (!pd || offset + 4 > pd->data.size()) return 0;
        return static_cast<uint32>(pd->data[offset])
             | static_cast<uint32>(pd->data[offset + 1]) << 8
             | static_cast<uint32>(pd->data[offset + 2]) << 16
             | static_cast<uint32>(pd->data[offset + 3]) << 24;
    }
    static uint32 PD_GetDataSize(PacketData* pd) { return pd ? static_cast<uint32>(pd->data.size()) : 0; }
    static void PD_ResetBitReader(PacketData* pd) { if (pd) pd->ResetBitReader(); }

    static std::string PD_ReadBytes(PacketData* pd, uint32 len)
    {
        return pd ? pd->ReadBytes(len) : "";
    }

    static std::string PD_ReadWoWString(PacketData* pd, uint32 len)
    {
        return pd ? pd->ReadWoWString(len) : "";
    }

    static uint64 PD_ReadPackedUInt64(PacketData* pd)
    {
        return pd ? pd->ReadPackedUInt64() : 0;
    }

    // PackedGuid128: [maskLow][maskHigh][non-zero low bytes][non-zero high bytes]
    static void PD_ReadPackedGuid(PacketData* pd, uint64& low, uint64& high)
    {
        if (!pd) { low = 0; high = 0; return; }
        pd->ReadPackedGuid(low, high);
    }

    static void PD_WritePackedUInt64(PacketData* pd, uint64 val)
    {
        // Reserve space for the mask byte, then write only non-zero bytes
        size_t maskPos = pd->data.size();
        pd->data.push_back(0); // placeholder for mask
        uint8 mask = 0;
        for (int i = 0; i < 8; ++i)
        {
            uint8 b = static_cast<uint8>(val >> (i * 8));
            if (b)
            {
                mask |= static_cast<uint8>(1 << i);
                pd->data.push_back(b);
            }
        }
        pd->data[maskPos] = mask;
    }
    // PackedGuid128 (TC/client format): [lowMask][highMask][low non-zero bytes][high non-zero bytes].
    // Delegates to PacketData::WritePackedGuid which writes both mask bytes first, matching the
    // client's ReadPackedGuid128. The previous inline implementation interleaved the masks
    // ([lowMask][lowBytes][highMask][highBytes]) which misaligned the client parser.
    static void PD_WritePackedGuid(PacketData* pd, uint64 low, uint64 high)
    {
        if (!pd) return;
        pd->WritePackedGuid(low, high);
    }

    // Helper to build full player GUID from low counter (dbId).
    // Returns both low and high parts of the 128-bit GUID.
    // High part encodes: HighGuid::Player, realm ID, subtype.
    static void BuildPlayerGuid(uint64 guidLow, uint64& outLow, uint64& outHigh)
    {
        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(guidLow);
        outLow = guid.GetCounter();
        outHigh = guid._data[1];  // Raw high 64-bit value
    }

    static void PD_WriteUInt8(PacketData* pd, uint8 v) { if (pd) pd->WriteUInt8(v); }
    static void PD_WriteUInt16(PacketData* pd, uint16 v) { if (pd) pd->WriteUInt16(v); }
    static void PD_WriteUInt32(PacketData* pd, uint32 v) { if (pd) pd->WriteUInt32(v); }
    static void PD_WriteUInt64(PacketData* pd, uint64 v) { if (pd) pd->WriteUInt64(v); }
    static void PD_WriteInt8(PacketData* pd, int8 v) { if (pd) pd->WriteInt8(v); }
    static void PD_WriteInt16(PacketData* pd, int16 v) { if (pd) pd->WriteInt16(v); }
    static void PD_WriteInt32(PacketData* pd, int32 v) { if (pd) pd->WriteInt32(v); }
    static void PD_WriteInt64(PacketData* pd, int64 v) { if (pd) pd->WriteInt64(v); }
    static void PD_WriteFloat(PacketData* pd, float v) { if (pd) pd->WriteFloat(v); }
    static void PD_WriteDouble(PacketData* pd, double v) { if (pd) pd->WriteDouble(v); }
    static void PD_WriteString(PacketData* pd, const std::string& v) { if (pd) pd->WriteString(v); }
    static void PD_WriteCString(PacketData* pd, const std::string& v) { if (pd) pd->WriteCString(v); }
    static void PD_WriteWoWString(PacketData* pd, const std::string& v, uint32 len) { if (pd) pd->WriteWoWString(v, len); }

    // Append bytes from a ByteBuffer into PacketData
    static void AppendBuf(PacketData* pd, ByteBuffer& buf)
    {
        for (size_t i = 0; i < buf.size(); ++i)
            pd->data.push_back(buf[i]);
    }

    // Write a WarbandGroupMember: slotIndex(u32) + memberType(i32) + contentSetID(i32) + [packedGuid if type==0]
    // Matches CharacterPackets.cpp operator<<(ByteBuffer&, WarbandGroupMember const&)
    static void PD_WriteWarbandMember(PacketData* pd, uint32 slotIndex, int32 memberType, int32 contentSetID, uint64 guidLow, uint64 guidHigh)
    {
        if (!pd) return;
        ByteBuffer buf;
        buf << uint32(slotIndex);
        buf << int32(memberType);
        buf << int32(contentSetID);
        if (memberType == 0)
        {
            // PackedGuid128: maskLow, maskHigh, then non-zero low bytes, then non-zero high bytes
            uint8 maskLow = 0, maskHigh = 0;
            uint8 lowBytes[8], highBytes[8];
            int lowCount = 0, highCount = 0;
            for (int b = 0; b < 8; ++b)
            {
                uint8 byte = (guidLow >> (b * 8)) & 0xFF;
                if (byte) { maskLow |= (1 << b); lowBytes[lowCount++] = byte; }
            }
            for (int b = 0; b < 8; ++b)
            {
                uint8 byte = (guidHigh >> (b * 8)) & 0xFF;
                if (byte) { maskHigh |= (1 << b); highBytes[highCount++] = byte; }
            }
            buf << uint8(maskLow) << uint8(maskHigh);
            for (int b = 0; b < lowCount; ++b)  buf << lowBytes[b];
            for (int b = 0; b < highCount; ++b) buf << highBytes[b];
        }
        AppendBuf(pd, buf);
    }

    // Write WarbandGroup header: groupID(u64)+orderIndex(u8)+sceneID(u32)+flags(u32)+contentSetID(i32)+memberCount(u32)
    // Matches CharacterPackets.cpp operator<<(ByteBuffer&, WarbandGroup const&) — header portion only
    static void PD_WriteWarbandGroupHeader(PacketData* pd,
        uint64 groupID, uint8 orderIndex, uint32 warbandSceneID, uint32 flags, int32 contentSetID, uint32 memberCount)
    {
        if (!pd) return;
        ByteBuffer buf;
        buf << uint64(groupID);
        buf << uint8(orderIndex);
        buf << uint32(warbandSceneID);
        buf << uint32(flags);
        buf << int32(contentSetID);
        buf << uint32(memberCount);
        AppendBuf(pd, buf);
    }

    // Write WarbandGroup name: WriteBits(len,9) + FlushBits + WriteString(data)
    // Matches: data << SizedString::BitsSize<9>(name); data.FlushBits(); data << SizedString::Data(name);
    static void PD_WriteWarbandGroupName(PacketData* pd, const std::string& name)
    {
        if (!pd) return;
        ByteBuffer buf;
        buf.WriteBits(static_cast<uint32>(name.length()), 9);
        buf.FlushBits();
        buf.WriteString(name);  // raw bytes, no null terminator
        AppendBuf(pd, buf);
    }

    static void PD_ResetReadPos(PacketData* pd) { if (pd) pd->ResetReadPos(); }
    static void PD_SetReadPos(PacketData* pd, uint32 pos) { if (pd) pd->SetReadPos(pos); }
    static void PD_ResetWritePos(PacketData* pd) { if (pd) pd->ResetWritePos(); }
    static uint32 PD_GetReadPos(PacketData* pd) { return pd ? static_cast<uint32>(pd->GetReadPos()) : 0; }
    static uint32 PD_GetWritePos(PacketData* pd) { return pd ? static_cast<uint32>(pd->GetWritePos()) : 0; }
    static bool PD_HasMoreData(PacketData* pd) { return pd ? pd->HasMoreData() : false; }
    static uint32 PD_GetSize(PacketData* pd) { return pd ? pd->size : 0; }
    static uint32 PD_GetOpcode(PacketData* pd) { return pd ? pd->opcode : 0; }

    // Bit operations
    static bool PD_ReadBit(PacketData* pd) { return pd ? pd->ReadBit() : false; }
    static uint32 PD_ReadBits(PacketData* pd, uint32 count) { return pd ? pd->ReadBits(count) : 0; }
    static void PD_WriteBit(PacketData* pd, bool bit) { if (pd) pd->WriteBit(bit); }
    static void PD_WriteBits(PacketData* pd, uint32 value, uint32 count) { if (pd) pd->WriteBits(value, count); }
    static void PD_FlushBits(PacketData* pd) { if (pd) pd->FlushBits(); }

    // ---- Send packet to player ----
    static void PD_SendToPlayer(Player* player, PacketData* pd)
    {
        if (!player || !player->GetSession() || !pd) return;
        WorldPacket packet(static_cast<OpcodeServer>(pd->opcode), pd->data.size());
        if (!pd->data.empty())
            packet.append(pd->data.data(), pd->data.size());
        player->GetSession()->SendPacket(&packet, true);
    }

    // ---- PacketData factory ----
    static PacketData* PD_Factory(uint32 opcode)
    {
        PacketData* pd = new PacketData();
        pd->opcode = opcode;
        return pd;
    }

    static void PD_Release(PacketData* /*pd*/)
    {
        // No-op since we use asOBJ_NOCOUNT
    }

    void RegisterPacketAPI(asIScriptEngine* _scriptEngine)
    {
        // Register WorldSession type
        int r = _scriptEngine->RegisterObjectType("WorldSession", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r >= 0 || r == asALREADY_REGISTERED)
        {
            _scriptEngine->RegisterObjectMethod("WorldSession", "uint32 GetAccountId() const",          asFUNCTION(Session_GetAccountId),          asCALL_CDECL_OBJFIRST);
            _scriptEngine->RegisterObjectMethod("WorldSession", "uint32 GetBattlenetAccountId() const",  asFUNCTION(Session_GetBattlenetAccountId), asCALL_CDECL_OBJFIRST);
            _scriptEngine->RegisterObjectMethod("WorldSession", "string GetAccountName() const",         asFUNCTION(Session_GetAccountName),        asCALL_CDECL_OBJFIRST);
            _scriptEngine->RegisterObjectMethod("WorldSession", "Player@ GetPlayer() const",             asFUNCTION(Session_GetPlayer),             asCALL_CDECL_OBJFIRST);
            _scriptEngine->RegisterObjectMethod("WorldSession", "void SendPacket(PacketData@)",          asFUNCTION(Session_SendPacket),            asCALL_CDECL_OBJFIRST);
        }

        // OpcodeCallback funcdef: bool handler(WorldSession@, PacketData@)
        _scriptEngine->RegisterFuncdef("bool OpcodeCallback(WorldSession@, PacketData@)");

        // Register opcode handler binding
        _scriptEngine->RegisterGlobalFunction("void RegisterOpcodeHandler(uint32, OpcodeCallback@, bool blockOriginal = false)", asFUNCTION(RegisterOpcodeHandler), asCALL_CDECL);
        _scriptEngine->RegisterGlobalFunction("void UnregisterOpcodeHandler(uint32)",                                           asFUNCTION(UnregisterOpcodeHandler), asCALL_CDECL);

        // Register PacketData type
        r = _scriptEngine->RegisterObjectType("PacketData", 0, asOBJ_REF | asOBJ_NOCOUNT);
        if (r < 0 && r != asALREADY_REGISTERED)
        {
            TC_LOG_ERROR("angelscript", "Failed to register PacketData type: {}", r);
            return;
        }

        // Read methods
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint8 ReadUInt8()", asFUNCTION(PD_ReadUInt8), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint16 ReadUInt16()", asFUNCTION(PD_ReadUInt16), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 ReadUInt32()", asFUNCTION(PD_ReadUInt32), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint64 ReadUInt64()", asFUNCTION(PD_ReadUInt64), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "int8 ReadInt8()", asFUNCTION(PD_ReadInt8), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "int16 ReadInt16()", asFUNCTION(PD_ReadInt16), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "int32 ReadInt32()", asFUNCTION(PD_ReadInt32), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "int64 ReadInt64()", asFUNCTION(PD_ReadInt64), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "float ReadFloat()", asFUNCTION(PD_ReadFloat), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "double ReadDouble()", asFUNCTION(PD_ReadDouble), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "string ReadString()", asFUNCTION(PD_ReadString), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "string ReadCString()", asFUNCTION(PD_ReadCString), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "string ReadBytes(uint32)",            asFUNCTION(PD_ReadBytes),       asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "string ReadWoWString(uint32)",        asFUNCTION(PD_ReadWoWString),   asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteUInt32At(uint32, uint32)",   asFUNCTION(PD_WriteUInt32At),   asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 ReadUInt32At(uint32) const",    asFUNCTION(PD_ReadUInt32At),    asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 GetDataSize() const",           asFUNCTION(PD_GetDataSize),     asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void ResetBitReader()",                  asFUNCTION(PD_ResetBitReader),    asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WritePackedGuid(uint64, uint64)",  asFUNCTION(PD_WritePackedGuid),   asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint64 ReadPackedUInt64()",             asFUNCTION(PD_ReadPackedUInt64),  asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void ReadPackedGuid(uint64 &out, uint64 &out)", asFUNCTION(PD_ReadPackedGuid), asCALL_CDECL_OBJFIRST);

        // Write methods
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteUInt8(uint8)", asFUNCTION(PD_WriteUInt8), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteUInt16(uint16)", asFUNCTION(PD_WriteUInt16), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteUInt32(uint32)", asFUNCTION(PD_WriteUInt32), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteUInt64(uint64)", asFUNCTION(PD_WriteUInt64), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteInt8(int8)", asFUNCTION(PD_WriteInt8), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteInt16(int16)", asFUNCTION(PD_WriteInt16), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteInt32(int32)", asFUNCTION(PD_WriteInt32), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteInt64(int64)", asFUNCTION(PD_WriteInt64), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteFloat(float)", asFUNCTION(PD_WriteFloat), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteDouble(double)", asFUNCTION(PD_WriteDouble), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteString(const string& in)", asFUNCTION(PD_WriteString), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteCString(const string& in)", asFUNCTION(PD_WriteCString), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteWoWString(const string& in, uint32)", asFUNCTION(PD_WriteWoWString), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteWarbandGroupHeader(uint64, uint8, uint32, uint32, int32, uint32)", asFUNCTION(PD_WriteWarbandGroupHeader), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteWarbandMember(uint32, int32, int32, uint64, uint64)", asFUNCTION(PD_WriteWarbandMember), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteWarbandGroupName(const string& in)", asFUNCTION(PD_WriteWarbandGroupName), asCALL_CDECL_OBJFIRST);

        // Position
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void ResetReadPos()", asFUNCTION(PD_ResetReadPos), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void SetReadPos(uint32)", asFUNCTION(PD_SetReadPos), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void ResetWritePos()", asFUNCTION(PD_ResetWritePos), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 GetReadPos() const", asFUNCTION(PD_GetReadPos), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 GetWritePos() const", asFUNCTION(PD_GetWritePos), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "bool HasMoreData() const", asFUNCTION(PD_HasMoreData), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 GetSize() const", asFUNCTION(PD_GetSize), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 GetOpcode() const", asFUNCTION(PD_GetOpcode), asCALL_CDECL_OBJFIRST);

        // Bit operations
        r = _scriptEngine->RegisterObjectMethod("PacketData", "bool ReadBit()", asFUNCTION(PD_ReadBit), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "uint32 ReadBits(uint32)", asFUNCTION(PD_ReadBits), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteBit(bool)", asFUNCTION(PD_WriteBit), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void WriteBits(uint32, uint32)", asFUNCTION(PD_WriteBits), asCALL_CDECL_OBJFIRST);
        r = _scriptEngine->RegisterObjectMethod("PacketData", "void FlushBits()", asFUNCTION(PD_FlushBits), asCALL_CDECL_OBJFIRST);

        // Factory
        r = _scriptEngine->RegisterGlobalFunction("PacketData@ CreatePacketData(uint32)", asFUNCTION(PD_Factory), asCALL_CDECL);

        // GUID helpers
        r = _scriptEngine->RegisterGlobalFunction("void BuildPlayerGuid(uint64, uint64 &out, uint64 &out)", asFUNCTION(BuildPlayerGuid), asCALL_CDECL);

        // Send
        r = _scriptEngine->RegisterGlobalFunction("void SendPacketToPlayer(Player@, PacketData@)", asFUNCTION(PD_SendToPlayer), asCALL_CDECL);

        TC_LOG_INFO("server.angelscript", "Packet API registered");
    }

    void RegisterEnhancedPacketAPI(asIScriptEngine* /*_scriptEngine*/)
    {
        // Already covered by RegisterPacketAPI — this is the consolidated version
        TC_LOG_INFO("server.angelscript", "Enhanced Packet API registered (consolidated with Packet API)");
    }

} // namespace AngelScript
