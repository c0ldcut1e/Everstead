#include "SpawnerDataS2CPacket.h"

#include "../../listener/PacketListener.h"

SpawnerDataS2CPacket::SpawnerDataS2CPacket()
{
    m_pos        = BlockPos(0, 0, 0);
    m_entityType = eEntityType_Pig;
}

SpawnerDataS2CPacket::SpawnerDataS2CPacket(const BlockPos &pos, eENTITY_TYPE entityType)
{
    m_pos        = pos;
    m_entityType = entityType;
}

SpawnerDataS2CPacket::~SpawnerDataS2CPacket()
{}

ePACKET_ID SpawnerDataS2CPacket::GetId()
{
    return ePacketId_SpawnerDataS2C;
}

void SpawnerDataS2CPacket::Read(DataInputStream *inputStream)
{
    int posX     = (int) inputStream->ReadInt();
    int posY     = (int) inputStream->ReadInt();
    int posZ     = (int) inputStream->ReadInt();
    m_pos        = BlockPos(posX, posY, posZ);
    m_entityType = (eENTITY_TYPE) inputStream->ReadShort();
}

void SpawnerDataS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteShort((uint16_t) m_entityType);
}

void SpawnerDataS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSpawnerData(this);
}

int SpawnerDataS2CPacket::GetEstimatedSize()
{
    return 14;
}

const BlockPos &SpawnerDataS2CPacket::GetPos() const
{
    return m_pos;
}

eENTITY_TYPE SpawnerDataS2CPacket::GetEntityType() const
{
    return m_entityType;
}
