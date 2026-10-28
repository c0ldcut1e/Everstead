#include "InteractC2SPacket.h"

#include "../../listener/PacketListener.h"

InteractC2SPacket::InteractC2SPacket()
{
    m_entityId = 0;
}

InteractC2SPacket::InteractC2SPacket(int entityId)
{
    m_entityId = entityId;
}

InteractC2SPacket::~InteractC2SPacket()
{}

ePACKET_ID InteractC2SPacket::GetId()
{
    return ePacketId_InteractC2S;
}

void InteractC2SPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
}

void InteractC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
}

void InteractC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleInteract(this);
}

int InteractC2SPacket::GetEstimatedSize()
{
    return 5;
}

int InteractC2SPacket::GetEntityId() const
{
    return m_entityId;
}
