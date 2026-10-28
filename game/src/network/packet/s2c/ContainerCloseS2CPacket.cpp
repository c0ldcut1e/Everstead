#include "ContainerCloseS2CPacket.h"

#include "../../listener/PacketListener.h"

ContainerCloseS2CPacket::ContainerCloseS2CPacket()
{
    m_menuId = 0;
}

ContainerCloseS2CPacket::ContainerCloseS2CPacket(uint8_t menuId)
{
    m_menuId = menuId;
}

ContainerCloseS2CPacket::~ContainerCloseS2CPacket()
{}

ePACKET_ID ContainerCloseS2CPacket::GetId()
{
    return ePacketId_ContainerCloseS2C;
}

void ContainerCloseS2CPacket::Read(DataInputStream *inputStream)
{
    m_menuId = inputStream->ReadByte();
}

void ContainerCloseS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_menuId);
}

void ContainerCloseS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleContainerClose(this);
}

int ContainerCloseS2CPacket::GetEstimatedSize()
{
    return 1;
}

uint8_t ContainerCloseS2CPacket::GetMenuId() const
{
    return m_menuId;
}
