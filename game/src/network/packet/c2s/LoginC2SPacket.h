#pragma once

#include <string>

#include "../../../utils/UniqueID.h"
#include "../../NetworkTypes.h"
#include "../Packet.h"

class LoginC2SPacket : public Packet
{
public:
    LoginC2SPacket();
    LoginC2SPacket(Pid pid, const std::string &username, const UniqueID &uniqueID);
    ~LoginC2SPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    Pid GetPid() const;
    const std::string &GetUsername() const;
    const UniqueID &GetUniqueID() const;

private:
    Pid m_pid;
    std::string m_username;
    UniqueID m_uniqueID;
};
