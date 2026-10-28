#pragma once

#include <cstdint>

#include "../../../world/item/ItemInstance.h"
#include "../Packet.h"

class SetEntityDataS2CPacket : public Packet
{
public:
    SetEntityDataS2CPacket();
    SetEntityDataS2CPacket(uint8_t playerIndex, uint8_t flags);
    SetEntityDataS2CPacket(int entityId, const ItemInstance &item, int age, int pickupDelay);
    ~SetEntityDataS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;
    uint8_t GetPlayerIndex() const;
    uint8_t GetFlags() const;
    const ItemInstance &GetItem() const;
    int GetAge() const;
    int GetPickupDelay() const;

private:
    // Zero targets a player by index
    int m_entityId;
    uint8_t m_playerIndex;
    uint8_t m_flags;
    ItemInstance m_item;
    int m_age;
    int m_pickupDelay;
};
