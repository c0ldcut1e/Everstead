#include "ContainerOpenS2CPacket.h"

#include "../../listener/PacketListener.h"

ContainerOpenS2CPacket::ContainerOpenS2CPacket()
{
    m_menuId = 0;
    m_rows   = 0;
}

ContainerOpenS2CPacket::ContainerOpenS2CPacket(uint8_t menuId, uint8_t rows)
{
    m_menuId = menuId;
    m_rows   = rows;
}

ContainerOpenS2CPacket::~ContainerOpenS2CPacket()
{}

ePACKET_ID ContainerOpenS2CPacket::GetId()
{
    return ePacketId_ContainerOpenS2C;
}

void ContainerOpenS2CPacket::Read(DataInputStream *inputStream)
{
    m_menuId = inputStream->ReadByte();
    m_rows   = inputStream->ReadByte();
}

void ContainerOpenS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_menuId);
    outputStream->WriteByte(m_rows);
}

void ContainerOpenS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleContainerOpen(this);
}

int ContainerOpenS2CPacket::GetEstimatedSize()
{
    return 2;
}

uint8_t ContainerOpenS2CPacket::GetMenuId() const
{
    return m_menuId;
}

uint8_t ContainerOpenS2CPacket::GetRows() const
{
    return m_rows;
}
