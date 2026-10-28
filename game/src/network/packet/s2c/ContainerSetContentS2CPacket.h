#pragma once

#include <vector>

#include "../Packet.h"

class ContainerSetContentS2CPacket : public Packet
{
public:
    ContainerSetContentS2CPacket();
    ContainerSetContentS2CPacket(uint8_t menuId, uint8_t selectedSlot, const std::vector<ItemInstance> &items, const ItemInstance &carried);
    ~ContainerSetContentS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetMenuId() const;
    uint8_t GetSelectedSlot() const;
    const ItemInstance &GetCarried() const;
    const std::vector<ItemInstance> &GetItems() const;

private:
    static const int s_maxSlots = 128;

    uint8_t m_menuId;
    uint8_t m_selectedSlot;
    std::vector<ItemInstance> m_items;
    ItemInstance m_carried;
};
