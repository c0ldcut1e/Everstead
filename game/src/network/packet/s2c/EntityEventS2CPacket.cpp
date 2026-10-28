#include "EntityEventS2CPacket.h"

#include "../../listener/PacketListener.h"

EntityEventS2CPacket::EntityEventS2CPacket()
{
    m_entityId = 0;
    m_event    = 0;
}

EntityEventS2CPacket::EntityEventS2CPacket(int entityId, uint8_t event)
{
    m_entityId = entityId;
    m_event    = event;
}

EntityEventS2CPacket::~EntityEventS2CPacket()
{}

ePACKET_ID EntityEventS2CPacket::GetId()
{
    return ePacketId_EntityEventS2C;
}

void EntityEventS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    m_event    = inputStream->ReadByte();
}

void EntityEventS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteByte(m_event);
}

void EntityEventS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleEntityEvent(this);
}

int EntityEventS2CPacket::GetEstimatedSize()
{
    return 6;
}

int EntityEventS2CPacket::GetEntityId() const
{
    return m_entityId;
}

uint8_t EntityEventS2CPacket::GetEvent() const
{
    return m_event;
}
