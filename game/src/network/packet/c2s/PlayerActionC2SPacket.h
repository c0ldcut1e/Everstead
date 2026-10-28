#pragma once

#include "../../../world/block/BlockPos.h"
#include "../Packet.h"

enum ePLAYER_ACTION : uint8_t
{
    ePlayerAction_Break          = 0,
    ePlayerAction_Place          = 1,
    ePlayerAction_StartSprinting = 2,
    ePlayerAction_StopSprinting  = 3,
    ePlayerAction_StartSneaking  = 4,
    ePlayerAction_StopSneaking   = 5,
    ePlayerAction_Use            = 6,
    ePlayerAction_UseItem        = 7,
    ePlayerAction_StartUsingItem = 8,
    ePlayerAction_ReleaseUseItem = 9,
    ePlayerAction_StartDestroy   = 10,
    ePlayerAction_AbortDestroy   = 11,
    ePlayerAction_StopSleeping   = 12,
};

class PlayerActionC2SPacket : public Packet
{
public:
    PlayerActionC2SPacket();
    PlayerActionC2SPacket(ePLAYER_ACTION action, const BlockPos &pos, uint8_t placedAgainstFace, uint16_t data);
    ~PlayerActionC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    ePLAYER_ACTION GetAction() const;
    const BlockPos &GetPos() const;
    uint8_t GetPlacedAgainstFace() const;
    uint16_t GetData() const;

private:
    ePLAYER_ACTION m_action;
    BlockPos m_pos;
    uint8_t m_placedAgainstFace;
    uint16_t m_data;
};
