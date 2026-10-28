#pragma once

#include <cstdint>

#include "../Packet.h"

class ContainerSetDataS2CPacket : public Packet
{
public:
    ContainerSetDataS2CPacket();
    ContainerSetDataS2CPacket(uint8_t menuId, uint8_t dataId, int16_t value);
    ~ContainerSetDataS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetMenuId() const;
    uint8_t GetDataId() const;
    int16_t GetValue() const;

private:
    uint8_t m_menuId;
    uint8_t m_dataId;
    int16_t m_value;
};
