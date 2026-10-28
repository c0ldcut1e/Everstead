#pragma once

#include <string>

#include "../../NetworkTypes.h"
#include "../Packet.h"

class PreLoginC2SPacket : public Packet
{
public:
    PreLoginC2SPacket();
    PreLoginC2SPacket(Pid pid, const std::string &username, uint16_t versionMajor, uint16_t versionMinor, uint16_t versionPatch);
    ~PreLoginC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    Pid GetPid() const;
    const std::string &GetUsername() const;
    uint16_t GetVersionMajor() const;
    uint16_t GetVersionMinor() const;
    uint16_t GetVersionPatch() const;

private:
    Pid m_pid;
    std::string m_username;
    uint16_t m_versionMajor;
    uint16_t m_versionMinor;
    uint16_t m_versionPatch;
};
