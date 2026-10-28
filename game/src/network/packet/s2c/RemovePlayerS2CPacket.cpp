#include "RemovePlayerS2CPacket.h"

#include "../../listener/PacketListener.h"

RemovePlayerS2CPacket::RemovePlayerS2CPacket()
{
    m_playerIndex = 0;
}

RemovePlayerS2CPacket::RemovePlayerS2CPacket(uint8_t playerIndex)
{
    m_playerIndex = playerIndex;
}

RemovePlayerS2CPacket::~RemovePlayerS2CPacket()
{
}

ePACKET_ID RemovePlayerS2CPacket::GetId()
{
    return ePacketId_RemovePlayerS2C;
}

void RemovePlayerS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
}

void RemovePlayerS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
}

void RemovePlayerS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleRemovePlayer(this);
}

int RemovePlayerS2CPacket::GetEstimatedSize()
{
    return 1;
}

uint8_t RemovePlayerS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}
