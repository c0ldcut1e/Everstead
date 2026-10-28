#include "KeepAliveS2CPacket.h"

#include "../../listener/PacketListener.h"

KeepAliveS2CPacket::KeepAliveS2CPacket()
{
}

KeepAliveS2CPacket::~KeepAliveS2CPacket()
{
}

ePACKET_ID KeepAliveS2CPacket::GetId()
{
    return ePacketId_KeepAliveS2C;
}

void KeepAliveS2CPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void KeepAliveS2CPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

void KeepAliveS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleKeepAlive(this);
}
