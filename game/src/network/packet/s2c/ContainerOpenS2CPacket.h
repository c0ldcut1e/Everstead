#pragma once

#include <cstdint>

#include "../Packet.h"

class ContainerOpenS2CPacket : public Packet
{
public:
    ContainerOpenS2CPacket();
    ContainerOpenS2CPacket(uint8_t menuId, uint8_t rows);
    ~ContainerOpenS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    uint8_t GetMenuId() const;
    uint8_t GetRows() const;

private:
    uint8_t m_menuId;
    uint8_t m_rows;
};
