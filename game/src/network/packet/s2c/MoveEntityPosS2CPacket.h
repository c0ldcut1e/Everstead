#pragma once

#include <cstdint>

#include "../Packet.h"

class MoveEntityPosS2CPacket : public Packet
{
public:
    MoveEntityPosS2CPacket();
    MoveEntityPosS2CPacket(int entityId, int16_t xa, int16_t ya, int16_t za, bool onGround);
    ~MoveEntityPosS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;

    int GetEntityId() const;
    int16_t GetXa() const;
    int16_t GetYa() const;
    int16_t GetZa() const;
    bool IsOnGround() const;

private:
    int m_entityId;
    int16_t m_xa;
    int16_t m_ya;
    int16_t m_za;
    bool m_onGround;
};
