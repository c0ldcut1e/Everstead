#include "SetCarriedItemC2SPacket.h"

#include "../../listener/PacketListener.h"

SetCarriedItemC2SPacket::SetCarriedItemC2SPacket()
{
    m_slot = 0;
}

SetCarriedItemC2SPacket::SetCarriedItemC2SPacket(uint8_t slot)
{
    m_slot = slot;
}

SetCarriedItemC2SPacket::~SetCarriedItemC2SPacket()
{}

ePACKET_ID SetCarriedItemC2SPacket::GetId()
{
    return ePacketId_SetCarriedItemC2S;
}

void SetCarriedItemC2SPacket::Read(DataInputStream *inputStream)
{
    m_slot = inputStream->ReadByte();
}

void SetCarriedItemC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_slot);
}

void SetCarriedItemC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleSetCarriedItem(this);
}

int SetCarriedItemC2SPacket::GetEstimatedSize()
{
    return 2;
}

uint8_t SetCarriedItemC2SPacket::GetSlot() const
{
    return m_slot;
}
