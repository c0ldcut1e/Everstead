#pragma once

#include <string>

#include "../Packet.h"

class ChatS2CPacket : public Packet
{
public:
    ChatS2CPacket();
    ChatS2CPacket(const std::string &message, bool advanced);
    ~ChatS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetMessage() const;
    bool IsAdvanced() const;

private:
    std::string m_message;
    bool m_advanced;
};
