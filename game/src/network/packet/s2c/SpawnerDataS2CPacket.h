#pragma once

#include <cstdint>

#include "../../../entity/EntityType.h"
#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

class SpawnerDataS2CPacket : public Packet
{
public:
    SpawnerDataS2CPacket();
    SpawnerDataS2CPacket(const BlockPos &pos, eENTITY_TYPE entityType);
    ~SpawnerDataS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const BlockPos &GetPos() const;
    eENTITY_TYPE GetEntityType() const;

private:
    BlockPos m_pos;
    eENTITY_TYPE m_entityType;
};
