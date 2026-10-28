#include "RenameItemC2SPacket.h"

#include "../../listener/PacketListener.h"

RenameItemC2SPacket::RenameItemC2SPacket()
{}

RenameItemC2SPacket::RenameItemC2SPacket(const std::string &name)
{
    m_name = name;
}

RenameItemC2SPacket::~RenameItemC2SPacket()
{}

ePACKET_ID RenameItemC2SPacket::GetId()
{
    return ePacketId_RenameItemC2S;
}

void RenameItemC2SPacket::Read(DataInputStream *inputStream)
{
    m_name = inputStream->ReadString();
}

void RenameItemC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_name);
}

void RenameItemC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleRenameItem(this);
}

int RenameItemC2SPacket::GetEstimatedSize()
{
    return 2 + (int) m_name.size();
}

const std::string &RenameItemC2SPacket::GetName() const
{
    return m_name;
}
