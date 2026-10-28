#include "LoginC2SPacket.h"

#include "../../listener/PacketListener.h"

LoginC2SPacket::LoginC2SPacket()
{
    m_pid      = 0;
    m_username = "";
    m_uniqueID = {};
}

LoginC2SPacket::LoginC2SPacket(Pid pid, const std::string &username, const UniqueID &uniqueID)
{
    m_pid      = pid;
    m_username = username;
    m_uniqueID = uniqueID;
}

LoginC2SPacket::~LoginC2SPacket()
{}

ePACKET_ID LoginC2SPacket::GetId()
{
    return ePacketId_LoginC2S;
}

void LoginC2SPacket::Read(DataInputStream *inputStream)
{
    m_pid            = inputStream->ReadInt();
    m_username       = inputStream->ReadString();
    uint32_t data[5] = {};
    for (int i = 0; i < 5; i++)
    {
        data[i] = inputStream->ReadInt();
    }

    m_uniqueID = UniqueID(data[0], data[1], data[2], data[3], data[4]);
}

void LoginC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt(m_pid);
    outputStream->WriteString(m_username);
    const uint32_t *data = m_uniqueID.GetData();
    for (int i = 0; i < 5; i++)
    {
        outputStream->WriteInt(data[i]);
    }
}

void LoginC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleLogin(this);
}

int LoginC2SPacket::GetEstimatedSize()
{
    return 26 + (int) m_username.size();
}

uint32_t LoginC2SPacket::GetPid() const
{
    return m_pid;
}

const std::string &LoginC2SPacket::GetUsername() const
{
    return m_username;
}

const UniqueID &LoginC2SPacket::GetUniqueID() const
{
    return m_uniqueID;
}
