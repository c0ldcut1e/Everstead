#include "ServerStatusS2CPacket.h"

#include "../../listener/PacketListener.h"

ServerStatusS2CPacket::ServerStatusS2CPacket()
{
    m_tps        = 0.0f;
    m_mspt       = 0.0f;
    m_maxPlayers = 0;
}

ServerStatusS2CPacket::ServerStatusS2CPacket(float tps, float mspt, uint8_t maxPlayers, const std::vector<PlayerInfo> &players)
{
    m_tps        = tps;
    m_mspt       = mspt;
    m_maxPlayers = maxPlayers;
    m_players    = players;
}

ServerStatusS2CPacket::~ServerStatusS2CPacket()
{}

ePACKET_ID ServerStatusS2CPacket::GetId()
{
    return ePacketId_ServerStatusS2C;
}

void ServerStatusS2CPacket::Read(DataInputStream *inputStream)
{
    m_tps         = inputStream->ReadFloat();
    m_mspt        = inputStream->ReadFloat();
    m_maxPlayers  = inputStream->ReadByte();
    uint8_t count = inputStream->ReadByte();
    m_players.resize(count);
    for (size_t i = 0; i < m_players.size(); i++)
    {
        m_players[i].playerIndex = inputStream->ReadByte();
        m_players[i].ping        = inputStream->ReadShort();
        m_players[i].isOperator  = inputStream->ReadBool();
        m_players[i].name        = inputStream->ReadString();
    }
}

void ServerStatusS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_tps);
    outputStream->WriteFloat(m_mspt);
    outputStream->WriteByte(m_maxPlayers);
    outputStream->WriteByte((uint8_t) m_players.size());
    for (size_t i = 0; i < m_players.size(); i++)
    {
        outputStream->WriteByte(m_players[i].playerIndex);
        outputStream->WriteShort(m_players[i].ping);
        outputStream->WriteBool(m_players[i].isOperator);
        outputStream->WriteString(m_players[i].name);
    }
}

void ServerStatusS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleServerStatus(this);
}

int ServerStatusS2CPacket::GetEstimatedSize()
{
    int size = 10;
    for (size_t i = 0; i < m_players.size(); i++)
    {
        size += 6 + (int) m_players[i].name.size();
    }

    return size;
}

float ServerStatusS2CPacket::GetTps() const
{
    return m_tps;
}

float ServerStatusS2CPacket::GetMspt() const
{
    return m_mspt;
}

uint8_t ServerStatusS2CPacket::GetMaxPlayers() const
{
    return m_maxPlayers;
}

const std::vector<ServerStatusS2CPacket::PlayerInfo> &ServerStatusS2CPacket::GetPlayers() const
{
    return m_players;
}
