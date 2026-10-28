#pragma once

#include <cstdint>

#include "../Packet.h"

class InteractC2SPacket : public Packet
{
public:
    InteractC2SPacket();
    explicit InteractC2SPacket(int entityId);
    ~InteractC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;

private:
    int m_entityId;
};
