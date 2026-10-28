#include "ChestOpenS2CPacket.h"

#include "../../listener/PacketListener.h"

ChestOpenS2CPacket::ChestOpenS2CPacket()
{
    m_pos       = BlockPos(0, 0, 0);
    m_openCount = 0;
}

ChestOpenS2CPacket::ChestOpenS2CPacket(const BlockPos &pos, uint8_t openCount)
{
    m_pos       = pos;
    m_openCount = openCount;
}

ChestOpenS2CPacket::~ChestOpenS2CPacket()
{}

ePACKET_ID ChestOpenS2CPacket::GetId()
{
    return ePacketId_ChestOpenS2C;
}

void ChestOpenS2CPacket::Read(DataInputStream *inputStream)
{
    int posX    = (int) inputStream->ReadInt();
    int posY    = (int) inputStream->ReadInt();
    int posZ    = (int) inputStream->ReadInt();
    m_pos       = BlockPos(posX, posY, posZ);
    m_openCount = inputStream->ReadByte();
}

void ChestOpenS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteByte(m_openCount);
}

void ChestOpenS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleChestOpen(this);
}

int ChestOpenS2CPacket::GetEstimatedSize()
{
    return 13;
}

const BlockPos &ChestOpenS2CPacket::GetPos() const
{
    return m_pos;
}

uint8_t ChestOpenS2CPacket::GetOpenCount() const
{
    return m_openCount;
}
