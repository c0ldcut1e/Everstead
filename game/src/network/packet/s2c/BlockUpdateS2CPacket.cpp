#include "BlockUpdateS2CPacket.h"

#include "../../listener/PacketListener.h"

BlockUpdateS2CPacket::BlockUpdateS2CPacket()
{
    m_pos       = BlockPos(0, 0, 0);
    m_blockId   = 0;
    m_blockData = 0;
}

BlockUpdateS2CPacket::BlockUpdateS2CPacket(const BlockPos &pos, uint16_t blockId, uint16_t blockData)
{
    m_pos       = pos;
    m_blockId   = blockId;
    m_blockData = blockData;
}

BlockUpdateS2CPacket::~BlockUpdateS2CPacket()
{}

ePACKET_ID BlockUpdateS2CPacket::GetId()
{
    return ePacketId_BlockUpdateS2C;
}

void BlockUpdateS2CPacket::Read(DataInputStream *inputStream)
{
    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);

    m_blockId   = inputStream->ReadShort();
    m_blockData = inputStream->ReadShort();
}

void BlockUpdateS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteShort(m_blockId);
    outputStream->WriteShort(m_blockData);
}

void BlockUpdateS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleBlockUpdate(this);
}

int BlockUpdateS2CPacket::GetEstimatedSize()
{
    return 16;
}

const BlockPos &BlockUpdateS2CPacket::GetPos() const
{
    return m_pos;
}

uint16_t BlockUpdateS2CPacket::GetBlockId() const
{
    return m_blockId;
}

uint16_t BlockUpdateS2CPacket::GetBlockData() const
{
    return m_blockData;
}
