#pragma once

#include <cstdint>

#include "../Packet.h"

class AnimateHurtS2CPacket : public Packet
{
public:
    AnimateHurtS2CPacket();
    explicit AnimateHurtS2CPacket(int entityId);
    ~AnimateHurtS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;

private:
    int m_entityId;
};
