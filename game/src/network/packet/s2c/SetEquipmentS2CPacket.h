#pragma once

#include "../Packet.h"

class SetEquipmentS2CPacket : public Packet
{
public:
    SetEquipmentS2CPacket();
    SetEquipmentS2CPacket(uint8_t playerIndex, uint8_t slot, const ItemInstance &item);
    ~SetEquipmentS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetPlayerIndex() const;
    uint8_t GetSlot() const;
    const ItemInstance &GetItem() const;

private:
    uint8_t m_playerIndex;
    uint8_t m_slot;
    ItemInstance m_item;
};
