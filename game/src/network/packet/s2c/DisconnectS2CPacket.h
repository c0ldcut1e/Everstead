#pragma once

#include <string>

#include "../DisconnectReason.h"
#include "../Packet.h"

class DisconnectS2CPacket : public Packet
{
public:
    DisconnectS2CPacket();
    explicit DisconnectS2CPacket(eDISCONNECT_REASON reason);
    DisconnectS2CPacket(eDISCONNECT_REASON reason, const std::wstring &text);
    ~DisconnectS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    eDISCONNECT_REASON GetReason() const;
    const std::wstring &GetText() const;

    static std::string GetReasonKey(eDISCONNECT_REASON reason);

private:
    eDISCONNECT_REASON m_reason;
    std::wstring m_text;
};
