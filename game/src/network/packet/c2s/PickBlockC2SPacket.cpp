#include "PickBlockC2SPacket.h"

#include "../../listener/PacketListener.h"

PickBlockC2SPacket::PickBlockC2SPacket()
{
    m_pos  = BlockPos(0, 0, 0);
    m_slot = 0;
}

PickBlockC2SPacket::PickBlockC2SPacket(const BlockPos &pos, uint8_t slot)
{
    m_pos  = pos;
    m_slot = slot;
}

PickBlockC2SPacket::~PickBlockC2SPacket()
{}

ePACKET_ID PickBlockC2SPacket::GetId()
{
    return ePacketId_PickBlockC2S;
}

void PickBlockC2SPacket::Read(DataInputStream *inputStream)
{
    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);
    m_slot   = inputStream->ReadByte();
}

void PickBlockC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteByte(m_slot);
}

void PickBlockC2SPacket::Handle(PacketListener *listener)
{
    listener->HandlePickBlock(this);
}

int PickBlockC2SPacket::GetEstimatedSize()
{
    return 13;
}

const BlockPos &PickBlockC2SPacket::GetPos() const
{
    return m_pos;
}

uint8_t PickBlockC2SPacket::GetSlot() const
{
    return m_slot;
}
