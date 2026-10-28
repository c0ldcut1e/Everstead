#include "SwingS2CPacket.h"

#include "../../listener/PacketListener.h"

SwingS2CPacket::SwingS2CPacket()
{
    m_playerIndex = 0;
}

SwingS2CPacket::SwingS2CPacket(uint8_t playerIndex)
{
    m_playerIndex = playerIndex;
}

SwingS2CPacket::~SwingS2CPacket()
{
}

ePACKET_ID SwingS2CPacket::GetId()
{
    return ePacketId_SwingS2C;
}

void SwingS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
}

void SwingS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
}

void SwingS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSwing(this);
}

int SwingS2CPacket::GetEstimatedSize()
{
    return 2;
}

uint8_t SwingS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}
