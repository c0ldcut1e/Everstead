#pragma once

#include <cstdint>

#include "../Packet.h"

class SetBoatDataS2CPacket : public Packet
{
public:
    SetBoatDataS2CPacket();
    SetBoatDataS2CPacket(int entityId, bool leftPaddle, bool rightPaddle);
    ~SetBoatDataS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    int GetEntityId() const;
    bool GetLeftPaddle() const;
    bool GetRightPaddle() const;

private:
    int m_entityId;
    bool m_leftPaddle;
    bool m_rightPaddle;
};
