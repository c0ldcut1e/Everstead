#pragma once

#include <cstdint>

#include "../Packet.h"

class ContainerCloseS2CPacket : public Packet
{
public:
    ContainerCloseS2CPacket();
    explicit ContainerCloseS2CPacket(uint8_t menuId);
    ~ContainerCloseS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetMenuId() const;

private:
    uint8_t m_menuId;
};
