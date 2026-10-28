#include "SetEquipmentS2CPacket.h"

#include "../../listener/PacketListener.h"

SetEquipmentS2CPacket::SetEquipmentS2CPacket()
{
    m_playerIndex = 0;
    m_slot        = 0;
}

SetEquipmentS2CPacket::SetEquipmentS2CPacket(uint8_t playerIndex, uint8_t slot, const ItemInstance &item)
{
    m_playerIndex = playerIndex;
    m_slot        = slot;
    m_item        = item;
}

SetEquipmentS2CPacket::~SetEquipmentS2CPacket()
{}

ePACKET_ID SetEquipmentS2CPacket::GetId()
{
    return ePacketId_SetEquipmentS2C;
}

void SetEquipmentS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    m_slot        = inputStream->ReadByte();
    m_item        = ReadItem(inputStream);
}

void SetEquipmentS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteByte(m_slot);
    WriteItem(outputStream, m_item);
}

void SetEquipmentS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetEquipment(this);
}

int SetEquipmentS2CPacket::GetEstimatedSize()
{
    return 8;
}

uint8_t SetEquipmentS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

uint8_t SetEquipmentS2CPacket::GetSlot() const
{
    return m_slot;
}

const ItemInstance &SetEquipmentS2CPacket::GetItem() const
{
    return m_item;
}
