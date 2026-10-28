#include "SetEntityDataS2CPacket.h"

#include "../../listener/PacketListener.h"

SetEntityDataS2CPacket::SetEntityDataS2CPacket()
{
    m_entityId    = 0;
    m_playerIndex = 0;
    m_flags       = 0;
    m_item        = ItemInstance();
    m_age         = 0;
    m_pickupDelay = 0;
}

SetEntityDataS2CPacket::SetEntityDataS2CPacket(uint8_t playerIndex, uint8_t flags)
{
    m_entityId    = 0;
    m_playerIndex = playerIndex;
    m_flags       = flags;
    m_item        = ItemInstance();
    m_age         = 0;
    m_pickupDelay = 0;
}

SetEntityDataS2CPacket::SetEntityDataS2CPacket(int entityId, const ItemInstance &item, int age, int pickupDelay)
{
    m_entityId    = entityId;
    m_playerIndex = 0;
    m_flags       = 0;
    m_item        = item;
    m_age         = age;
    m_pickupDelay = pickupDelay;
}

SetEntityDataS2CPacket::~SetEntityDataS2CPacket()
{}

ePACKET_ID SetEntityDataS2CPacket::GetId()
{
    return ePacketId_SetEntityDataS2C;
}

void SetEntityDataS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
    if (m_entityId == 0)
    {
        m_playerIndex = inputStream->ReadByte();
        m_flags       = inputStream->ReadByte();
        return;
    }

    m_item        = ReadItem(inputStream);
    m_age         = (int) inputStream->ReadShort();
    m_pickupDelay = (int) inputStream->ReadShort();
}

void SetEntityDataS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    if (m_entityId == 0)
    {
        outputStream->WriteByte(m_playerIndex);
        outputStream->WriteByte(m_flags);
        return;
    }

    WriteItem(outputStream, m_item);
    outputStream->WriteShort((uint16_t) m_age);
    outputStream->WriteShort((uint16_t) m_pickupDelay);
}

void SetEntityDataS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetEntityData(this);
}

int SetEntityDataS2CPacket::GetEstimatedSize()
{
    return m_entityId == 0 ? 7 : 17;
}

int SetEntityDataS2CPacket::GetEntityId() const
{
    return m_entityId;
}

uint8_t SetEntityDataS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

uint8_t SetEntityDataS2CPacket::GetFlags() const
{
    return m_flags;
}

const ItemInstance &SetEntityDataS2CPacket::GetItem() const
{
    return m_item;
}

int SetEntityDataS2CPacket::GetAge() const
{
    return m_age;
}

int SetEntityDataS2CPacket::GetPickupDelay() const
{
    return m_pickupDelay;
}
