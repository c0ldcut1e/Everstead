#pragma once

#include <cstdint>
#include <vector>

#include "../Packet.h"

class RemoveEntitiesS2CPacket : public Packet
{
public:
    RemoveEntitiesS2CPacket();
    explicit RemoveEntitiesS2CPacket(const std::vector<int> &entityIds);
    ~RemoveEntitiesS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::vector<int> &GetEntityIds() const;

private:
    std::vector<int> m_entityIds;
};
