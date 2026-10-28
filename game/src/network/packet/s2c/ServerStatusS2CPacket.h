#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../Packet.h"

class ServerStatusS2CPacket : public Packet
{
public:
    struct PlayerInfo
    {
        uint8_t playerIndex;
        uint16_t ping;
        bool isOperator;
        std::string name;
    };

    ServerStatusS2CPacket();
    ServerStatusS2CPacket(float tps, float mspt, uint8_t maxPlayers, const std::vector<PlayerInfo> &players);
    ~ServerStatusS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    float GetTps() const;
    float GetMspt() const;
    uint8_t GetMaxPlayers() const;
    const std::vector<PlayerInfo> &GetPlayers() const;

private:
    float m_tps;
    float m_mspt;
    uint8_t m_maxPlayers;
    std::vector<PlayerInfo> m_players;
};
