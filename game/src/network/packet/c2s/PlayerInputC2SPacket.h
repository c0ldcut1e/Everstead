#pragma once

#include "../Packet.h"

class PlayerInputC2SPacket : public Packet
{
public:
    PlayerInputC2SPacket();
    explicit PlayerInputC2SPacket(float forward);
    ~PlayerInputC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    float GetForward() const;

private:
    float m_forward;
};
