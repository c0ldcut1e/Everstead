#include "TakeItemEntityS2CPacket.h"

#include "../../listener/PacketListener.h"

TakeItemEntityS2CPacket::TakeItemEntityS2CPacket()
{
    m_entityId    = 0;
    m_playerIndex = 0;
    m_count       = 0;
}

TakeItemEntityS2CPacket::TakeItemEntityS2CPacket(int entityId, uint8_t playerIndex, int count)
{
    m_entityId    = entityId;
    m_playerIndex = playerIndex;
    m_count       = count;
}

TakeItemEntityS2CPacket::~TakeItemEntityS2CPacket()
{}

ePACKET_ID TakeItemEntityS2CPacket::GetId()
{
    return ePacketId_TakeItemEntityS2C;
}

void TakeItemEntityS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId    = (int) inputStream->ReadInt();
    m_playerIndex = inputStream->ReadByte();
    m_count       = (int) inputStream->ReadShort();
}

void TakeItemEntityS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteShort((uint16_t) m_count);
}

void TakeItemEntityS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleTakeItemEntity(this);
}

int TakeItemEntityS2CPacket::GetEstimatedSize()
{
    return 7;
}

int TakeItemEntityS2CPacket::GetEntityId() const
{
    return m_entityId;
}

uint8_t TakeItemEntityS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

int TakeItemEntityS2CPacket::GetCount() const
{
    return m_count;
}
