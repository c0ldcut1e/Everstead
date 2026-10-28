#pragma once

#include <string>

#include "../Packet.h"

class RenameItemC2SPacket : public Packet
{
public:
    RenameItemC2SPacket();
    explicit RenameItemC2SPacket(const std::string &name);
    ~RenameItemC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    const std::string &GetName() const;

private:
    std::string m_name;
};
