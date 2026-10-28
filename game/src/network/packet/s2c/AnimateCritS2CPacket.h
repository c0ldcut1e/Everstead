#pragma once

#include <cstdint>

#include "../Packet.h"

class AnimateCritS2CPacket : public Packet
{
public:
    AnimateCritS2CPacket();
    explicit AnimateCritS2CPacket(int entityId);
    ~AnimateCritS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;

private:
    int m_entityId;
};
