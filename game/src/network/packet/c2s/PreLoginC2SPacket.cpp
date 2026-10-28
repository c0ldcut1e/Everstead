#include "PreLoginC2SPacket.h"

#include "../../listener/PacketListener.h"

PreLoginC2SPacket::PreLoginC2SPacket()
{
    m_pid          = 0;
    m_username     = "";
    m_versionMajor = 0;
    m_versionMinor = 0;
    m_versionPatch = 0;
}

PreLoginC2SPacket::PreLoginC2SPacket(Pid pid, const std::string &username, uint16_t versionMajor, uint16_t versionMinor, uint16_t versionPatch)
{
    m_pid          = pid;
    m_username     = username;
    m_versionMajor = versionMajor;
    m_versionMinor = versionMinor;
    m_versionPatch = versionPatch;
}

PreLoginC2SPacket::~PreLoginC2SPacket()
{}

ePACKET_ID PreLoginC2SPacket::GetId()
{
    return ePacketId_PreLoginC2S;
}

void PreLoginC2SPacket::Read(DataInputStream *inputStream)
{
    m_pid          = inputStream->ReadInt();
    m_username     = inputStream->ReadString();
    m_versionMajor = inputStream->ReadShort();
    m_versionMinor = inputStream->ReadShort();
    m_versionPatch = inputStream->ReadShort();
}

void PreLoginC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt(m_pid);
    outputStream->WriteString(m_username);
    outputStream->WriteShort(m_versionMajor);
    outputStream->WriteShort(m_versionMinor);
    outputStream->WriteShort(m_versionPatch);
}

void PreLoginC2SPacket::Handle(PacketListener *listener)
{
    listener->HandlePreLogin(this);
}

int PreLoginC2SPacket::GetEstimatedSize()
{
    return 12 + (int) m_username.size();
}

uint32_t PreLoginC2SPacket::GetPid() const
{
    return m_pid;
}

const std::string &PreLoginC2SPacket::GetUsername() const
{
    return m_username;
}

uint16_t PreLoginC2SPacket::GetVersionMajor() const
{
    return m_versionMajor;
}

uint16_t PreLoginC2SPacket::GetVersionMinor() const
{
    return m_versionMinor;
}

uint16_t PreLoginC2SPacket::GetVersionPatch() const
{
    return m_versionPatch;
}
