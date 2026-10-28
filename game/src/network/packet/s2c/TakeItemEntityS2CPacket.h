#pragma once

#include <cstdint>

#include "../Packet.h"

class TakeItemEntityS2CPacket : public Packet
{
public:
    TakeItemEntityS2CPacket();
    TakeItemEntityS2CPacket(int entityId, uint8_t playerIndex, int count);
    ~TakeItemEntityS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;
    uint8_t GetPlayerIndex() const;
    int GetCount() const;

private:
    int m_entityId;
    uint8_t m_playerIndex;
    int m_count;
};
