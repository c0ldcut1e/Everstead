#include "ContainerSetSlotS2CPacket.h"

#include "../../listener/PacketListener.h"

ContainerSetSlotS2CPacket::ContainerSetSlotS2CPacket()
{
    m_slot = 0;
}

ContainerSetSlotS2CPacket::ContainerSetSlotS2CPacket(uint8_t slot, const ItemInstance &item)
{
    m_slot = slot;
    m_item = item;
}

ContainerSetSlotS2CPacket::~ContainerSetSlotS2CPacket()
{}

ePACKET_ID ContainerSetSlotS2CPacket::GetId()
{
    return ePacketId_ContainerSetSlotS2C;
}

void ContainerSetSlotS2CPacket::Read(DataInputStream *inputStream)
{
    m_slot = inputStream->ReadByte();
    m_item = ReadItem(inputStream);
}

void ContainerSetSlotS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_slot);
    WriteItem(outputStream, m_item);
}

void ContainerSetSlotS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleContainerSetSlot(this);
}

int ContainerSetSlotS2CPacket::GetEstimatedSize()
{
    return 7;
}

uint8_t ContainerSetSlotS2CPacket::GetSlot() const
{
    return m_slot;
}

const ItemInstance &ContainerSetSlotS2CPacket::GetItem() const
{
    return m_item;
}
