#include "RemoveEntitiesS2CPacket.h"

#include "../../listener/PacketListener.h"

RemoveEntitiesS2CPacket::RemoveEntitiesS2CPacket()
{
    m_entityIds = {};
}

RemoveEntitiesS2CPacket::RemoveEntitiesS2CPacket(const std::vector<int> &entityIds)
{
    m_entityIds = entityIds;
}

RemoveEntitiesS2CPacket::~RemoveEntitiesS2CPacket()
{}

ePACKET_ID RemoveEntitiesS2CPacket::GetId()
{
    return ePacketId_RemoveEntitiesS2C;
}

void RemoveEntitiesS2CPacket::Read(DataInputStream *inputStream)
{
    uint16_t count = inputStream->ReadShort();
    m_entityIds.resize(count);
    for (size_t i = 0; i < m_entityIds.size(); i++)
    {
        m_entityIds[i] = (int) inputStream->ReadInt();
    }
}

void RemoveEntitiesS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteShort((uint16_t) m_entityIds.size());
    for (size_t i = 0; i < m_entityIds.size(); i++)
    {
        outputStream->WriteInt((uint32_t) m_entityIds[i]);
    }
}

void RemoveEntitiesS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleRemoveEntities(this);
}

int RemoveEntitiesS2CPacket::GetEstimatedSize()
{
    return 2 + (int) m_entityIds.size() * 4;
}

const std::vector<int> &RemoveEntitiesS2CPacket::GetEntityIds() const
{
    return m_entityIds;
}
