#pragma once

#include <string>

#include "../Packet.h"

class ChatC2SPacket : public Packet
{
public:
    ChatC2SPacket();
    explicit ChatC2SPacket(const std::string &message);
    ~ChatC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetMessage() const;

private:
    std::string m_message;
};
