#pragma once

#include "../../../entity/Mob.h"
#include "../Packet.h"

class SetMobDataS2CPacket : public Packet
{
public:
    SetMobDataS2CPacket();
    SetMobDataS2CPacket(int entityId, const MobData &data);
    ~SetMobDataS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;
    const MobData &GetData() const;

private:
    int m_entityId;
    MobData m_data;
};
