#pragma once

#include <cstdint>
#include <string>

#include "../Packet.h"

class InfoS2CPacket : public Packet
{
public:
    InfoS2CPacket();
    InfoS2CPacket(uint8_t playerIndex, int entityId, uint8_t maxPlayers, uint8_t playerCount, const std::string &worldName,
                  const std::string &serverName);
    ~InfoS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;
    int GetEntityId() const;
    uint8_t GetMaxPlayers() const;
    uint8_t GetPlayerCount() const;
    const std::string &GetWorldName() const;
    const std::string &GetServerName() const;

private:
    uint8_t m_playerIndex;
    int m_entityId;
    uint8_t m_maxPlayers;
    uint8_t m_playerCount;
    std::string m_worldName;
    std::string m_serverName;
};
