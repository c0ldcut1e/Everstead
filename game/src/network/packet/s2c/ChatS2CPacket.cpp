#include "ChatS2CPacket.h"

#include "../../listener/PacketListener.h"

ChatS2CPacket::ChatS2CPacket()
{
    m_message  = "";
    m_advanced = false;
}

ChatS2CPacket::ChatS2CPacket(const std::string &message, bool advanced)
{
    m_message  = message;
    m_advanced = advanced;
}

ChatS2CPacket::~ChatS2CPacket()
{
}

ePACKET_ID ChatS2CPacket::GetId()
{
    return ePacketId_ChatS2C;
}

void ChatS2CPacket::Read(DataInputStream *inputStream)
{
    m_message  = inputStream->ReadString();
    m_advanced = inputStream->ReadBool();
}

void ChatS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_message);
    outputStream->WriteBool(m_advanced);
}

void ChatS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleChat(this);
}

int ChatS2CPacket::GetEstimatedSize()
{
    return 5 + (int) m_message.size();
}

const std::string &ChatS2CPacket::GetMessage() const
{
    return m_message;
}

bool ChatS2CPacket::IsAdvanced() const
{
    return m_advanced;
}
