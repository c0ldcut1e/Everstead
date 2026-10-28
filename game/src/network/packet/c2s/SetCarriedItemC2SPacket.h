#pragma once

#include "../Packet.h"

class SetCarriedItemC2SPacket : public Packet
{
public:
    SetCarriedItemC2SPacket();
    explicit SetCarriedItemC2SPacket(uint8_t slot);
    ~SetCarriedItemC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetSlot() const;

private:
    uint8_t m_slot;
};
