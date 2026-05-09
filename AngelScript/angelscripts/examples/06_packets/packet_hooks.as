/*
 * Example 06: Packet Hooks
 * Intercept incoming and outgoing packets.
 * Log specific opcodes, block unwanted packets.
 */

#include "../includes/ScriptFramework.as"

bool OnPacketReceive(WorldSession@ session, WorldPacket@ packet, uint32 opcode)
{
    if (session is null) return false;

    // Log specific opcodes
    if (opcode == 0x34F7) // CMSG_CAST_SPELL
    {
        Print("[Packet] CastSpell from " + session.GetAccountId());
    }

    // return true to block the packet from being processed
    return false;
}

void OnPacketSend(WorldSession@ session, WorldPacket@ packet, uint32 opcode)
{
    if (session is null) return;

    // Log outgoing SMSG packets
    Print("[PacketSend] opcode=" + opcode);
}

void main()
{
    RegisterPacketScript(PACKET_ON_RECEIVE, @OnPacketReceive);
    RegisterPacketScript(PACKET_ON_SEND,    @OnPacketSend);
    Print("[Packets] Receive + Send hooks registered");
}
