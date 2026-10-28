#include "PlayerDeathS2CPacket.h"

#include "../../listener/PacketListener.h"

PlayerDeathS2CPacket::PlayerDeathS2CPacket()
{
    m_playerIndex = 0;
    m_message     = "";
}

PlayerDeathS2CPacket::PlayerDeathS2CPacket(uint8_t playerIndex, const std::string &message)
{
    m_playerIndex = playerIndex;
    m_message     = message;
}

PlayerDeathS2CPacket::~PlayerDeathS2CPacket()
{}

ePACKET_ID PlayerDeathS2CPacket::GetId()
{
    return ePacketId_PlayerDeathS2C;
}

void PlayerDeathS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    m_message     = inputStream->ReadString();
}

void PlayerDeathS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteString(m_message);
}

void PlayerDeathS2CPacket::Handle(PacketListener *listener)
{
    listener->HandlePlayerDeath(this);
}

int PlayerDeathS2CPacket::GetEstimatedSize()
{
    return 6 + (int) m_message.size();
}

uint8_t PlayerDeathS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

const std::string &PlayerDeathS2CPacket::GetMessage() const
{
    return m_message;
}
