#pragma once

#include <cstdint>

#include "../Packet.h"

class EntityEventS2CPacket : public Packet
{
public:
    EntityEventS2CPacket();
    EntityEventS2CPacket(int entityId, uint8_t event);
    ~EntityEventS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;
    uint8_t GetEvent() const;

private:
    int m_entityId;
    uint8_t m_event;
};
