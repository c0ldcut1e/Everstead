#pragma once

#include <cstdint>
#include <string>

#include "../Packet.h"

class AddPlayerS2CPacket : public Packet
{
public:
    AddPlayerS2CPacket();
    AddPlayerS2CPacket(uint8_t playerIndex, int entityId, const std::string &username, double x, double y, double z, float yaw, float pitch,
                       uint8_t flags);
    ~AddPlayerS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;
    int GetEntityId() const;
    const std::string &GetUsername() const;
    double GetX() const;
    double GetY() const;
    double GetZ() const;
    float GetYaw() const;
    float GetPitch() const;
    uint8_t GetFlags() const;

private:
    uint8_t m_playerIndex;
    int m_entityId;
    std::string m_username;
    double m_x;
    double m_y;
    double m_z;
    float m_yaw;
    float m_pitch;
    uint8_t m_flags;
};
