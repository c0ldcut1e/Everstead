#include "ContainerSetDataS2CPacket.h"

#include "../../listener/PacketListener.h"

ContainerSetDataS2CPacket::ContainerSetDataS2CPacket()
{
    m_menuId = 0;
    m_dataId = 0;
    m_value  = 0;
}

ContainerSetDataS2CPacket::ContainerSetDataS2CPacket(uint8_t menuId, uint8_t dataId, int16_t value)
{
    m_menuId = menuId;
    m_dataId = dataId;
    m_value  = value;
}

ContainerSetDataS2CPacket::~ContainerSetDataS2CPacket()
{}

ePACKET_ID ContainerSetDataS2CPacket::GetId()
{
    return ePacketId_ContainerSetDataS2C;
}

void ContainerSetDataS2CPacket::Read(DataInputStream *inputStream)
{
    m_menuId = inputStream->ReadByte();
    m_dataId = inputStream->ReadByte();
    m_value  = (int16_t) inputStream->ReadShort();
}

void ContainerSetDataS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_menuId);
    outputStream->WriteByte(m_dataId);
    outputStream->WriteShort((uint16_t) m_value);
}

void ContainerSetDataS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleContainerSetData(this);
}

int ContainerSetDataS2CPacket::GetEstimatedSize()
{
    return 4;
}

uint8_t ContainerSetDataS2CPacket::GetMenuId() const
{
    return m_menuId;
}

uint8_t ContainerSetDataS2CPacket::GetDataId() const
{
    return m_dataId;
}

int16_t ContainerSetDataS2CPacket::GetValue() const
{
    return m_value;
}
