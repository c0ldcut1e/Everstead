#pragma once

#include "../Packet.h"

class SetHealthS2CPacket : public Packet
{
public:
    SetHealthS2CPacket();
    SetHealthS2CPacket(int health, int foodLevel, float saturationLevel);
    ~SetHealthS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetHealth() const;
    int GetFoodLevel() const;
    float GetSaturationLevel() const;

private:
    int m_health;
    int m_foodLevel;
    float m_saturationLevel;
};
