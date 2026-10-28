#pragma once

#include <cstdint>

#include "../Packet.h"

class AttackC2SPacket : public Packet
{
public:
    AttackC2SPacket();
    explicit AttackC2SPacket(int entityId);
    ~AttackC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;

private:
    int m_entityId;
};
