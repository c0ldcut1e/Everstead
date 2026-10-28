#include "BlockDestructionS2CPacket.h"

#include "../../listener/PacketListener.h"

BlockDestructionS2CPacket::BlockDestructionS2CPacket()
{
    m_playerIndex = 0;
    m_pos         = BlockPos(0, 0, 0);
    m_stage       = -1;
}

BlockDestructionS2CPacket::BlockDestructionS2CPacket(uint8_t playerIndex, const BlockPos &pos, int stage)
{
    m_playerIndex = playerIndex;
    m_pos         = pos;
    m_stage       = stage;
}

BlockDestructionS2CPacket::~BlockDestructionS2CPacket()
{}

ePACKET_ID BlockDestructionS2CPacket::GetId()
{
    return ePacketId_BlockDestructionS2C;
}

void BlockDestructionS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    int posX      = (int) inputStream->ReadInt();
    int posY      = (int) inputStream->ReadInt();
    int posZ      = (int) inputStream->ReadInt();
    m_pos         = BlockPos(posX, posY, posZ);
    m_stage       = (int8_t) inputStream->ReadByte();
}

void BlockDestructionS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteByte((uint8_t) m_stage);
}

void BlockDestructionS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleBlockDestruction(this);
}

int BlockDestructionS2CPacket::GetEstimatedSize()
{
    return 15;
}

uint8_t BlockDestructionS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

const BlockPos &BlockDestructionS2CPacket::GetPos() const
{
    return m_pos;
}

int BlockDestructionS2CPacket::GetStage() const
{
    return m_stage;
}
