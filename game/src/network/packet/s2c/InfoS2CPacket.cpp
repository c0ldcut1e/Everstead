#include "InfoS2CPacket.h"

#include "../../listener/PacketListener.h"

InfoS2CPacket::InfoS2CPacket()
{
    m_playerIndex = 0;
    m_entityId    = 0;
    m_maxPlayers  = 0;
    m_playerCount = 0;
    m_worldName   = "";
    m_serverName  = "";
}

InfoS2CPacket::InfoS2CPacket(uint8_t playerIndex, int entityId, uint8_t maxPlayers, uint8_t playerCount, const std::string &worldName,
                             const std::string &serverName)
{
    m_playerIndex = playerIndex;
    m_entityId    = entityId;
    m_maxPlayers  = maxPlayers;
    m_playerCount = playerCount;
    m_worldName   = worldName;
    m_serverName  = serverName;
}

InfoS2CPacket::~InfoS2CPacket()
{}

ePACKET_ID InfoS2CPacket::GetId()
{
    return ePacketId_InfoS2C;
}

void InfoS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    m_entityId    = (int) inputStream->ReadInt();
    m_maxPlayers  = inputStream->ReadByte();
    m_playerCount = inputStream->ReadByte();
    m_worldName   = inputStream->ReadString();
    m_serverName  = inputStream->ReadString();
}

void InfoS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteByte(m_maxPlayers);
    outputStream->WriteByte(m_playerCount);
    outputStream->WriteString(m_worldName);
    outputStream->WriteString(m_serverName);
}

void InfoS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleInfo(this);
}

int InfoS2CPacket::GetEstimatedSize()
{
    return 11 + (int) m_worldName.size() + (int) m_serverName.size();
}

uint8_t InfoS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

int InfoS2CPacket::GetEntityId() const
{
    return m_entityId;
}

uint8_t InfoS2CPacket::GetMaxPlayers() const
{
    return m_maxPlayers;
}

uint8_t InfoS2CPacket::GetPlayerCount() const
{
    return m_playerCount;
}

const std::string &InfoS2CPacket::GetWorldName() const
{
    return m_worldName;
}

const std::string &InfoS2CPacket::GetServerName() const
{
    return m_serverName;
}
