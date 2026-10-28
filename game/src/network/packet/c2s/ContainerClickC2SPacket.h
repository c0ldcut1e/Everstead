#pragma once

#include <vector>

#include "../../../world/inventory/InventoryMenu.h"
#include "../Packet.h"

class ContainerClickC2SPacket : public Packet
{
public:
    ContainerClickC2SPacket();
    ContainerClickC2SPacket(uint8_t menuId, int16_t slot, uint8_t button, eCLICK_TYPE type);
    ContainerClickC2SPacket(uint8_t menuId, const std::vector<int> &dragSlots, uint8_t button);
    ~ContainerClickC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetMenuId() const;
    int16_t GetSlot() const;
    uint8_t GetButton() const;
    eCLICK_TYPE GetType() const;
    const std::vector<int> &GetDragSlots() const;

private:
    static const int s_maxDragSlots = 128;

    uint8_t m_menuId;
    int16_t m_slot;
    uint8_t m_button;
    eCLICK_TYPE m_type;
    std::vector<int> m_dragSlots;
};
