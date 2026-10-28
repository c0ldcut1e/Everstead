#pragma once

#include "../../../world/DimensionId.h"
#include "../Packet.h"

class ChangeDimensionS2CPacket : public Packet
{
public:
    ChangeDimensionS2CPacket();
    ChangeDimensionS2CPacket(eDIMENSION_ID dimension, float stability);
    ~ChangeDimensionS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    eDIMENSION_ID GetDimension() const;
    float GetStability() const;

private:
    eDIMENSION_ID m_dimension;
    float m_stability;
};
