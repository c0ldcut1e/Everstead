#include "KeepAliveC2SPacket.h"

#include "../../listener/PacketListener.h"

KeepAliveC2SPacket::KeepAliveC2SPacket()
{
}

KeepAliveC2SPacket::~KeepAliveC2SPacket()
{
}

ePACKET_ID KeepAliveC2SPacket::GetId()
{
    return ePacketId_KeepAliveC2S;
}

void KeepAliveC2SPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void KeepAliveC2SPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

void KeepAliveC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleKeepAlive(this);
}
