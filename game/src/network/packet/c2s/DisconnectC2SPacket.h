#pragma once

#include "../DisconnectReason.h"
#include "../Packet.h"

class DisconnectC2SPacket : public Packet
{
public:
    DisconnectC2SPacket();
    explicit DisconnectC2SPacket(eDISCONNECT_REASON reason);
    ~DisconnectC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    eDISCONNECT_REASON GetReason() const;

private:
    eDISCONNECT_REASON m_reason;
};
