#include "ChatC2SPacket.h"

#include "../../listener/PacketListener.h"

ChatC2SPacket::ChatC2SPacket()
{
    m_message = "";
}

ChatC2SPacket::ChatC2SPacket(const std::string &message)
{
    m_message = message;
}

ChatC2SPacket::~ChatC2SPacket()
{
}

ePACKET_ID ChatC2SPacket::GetId()
{
    return ePacketId_ChatC2S;
}

void ChatC2SPacket::Read(DataInputStream *inputStream)
{
    m_message = inputStream->ReadString();
}

void ChatC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_message);
}

void ChatC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleChat(this);
}

int ChatC2SPacket::GetEstimatedSize()
{
    return 4 + (int) m_message.size();
}

const std::string &ChatC2SPacket::GetMessage() const
{
    return m_message;
}
