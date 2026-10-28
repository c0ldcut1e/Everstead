#pragma once

#include "../Packet.h"

class ContainerSetSlotS2CPacket : public Packet
{
public:
    ContainerSetSlotS2CPacket();
    ContainerSetSlotS2CPacket(uint8_t slot, const ItemInstance &item);
    ~ContainerSetSlotS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetSlot() const;
    const ItemInstance &GetItem() const;

private:
    uint8_t m_slot;
    ItemInstance m_item;
};
