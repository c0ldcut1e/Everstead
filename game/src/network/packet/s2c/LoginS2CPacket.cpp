#include "LoginS2CPacket.h"

#include "../../listener/PacketListener.h"

LoginS2CPacket::LoginS2CPacket()
{
}

LoginS2CPacket::~LoginS2CPacket()
{
}

ePACKET_ID LoginS2CPacket::GetId()
{
    return ePacketId_LoginS2C;
}

void LoginS2CPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void LoginS2CPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

void LoginS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleLogin(this);
}
