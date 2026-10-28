#include "RespawnC2SPacket.h"

#include "../../listener/PacketListener.h"

RespawnC2SPacket::RespawnC2SPacket()
{}

RespawnC2SPacket::~RespawnC2SPacket()
{}

ePACKET_ID RespawnC2SPacket::GetId()
{
    return ePacketId_RespawnC2S;
}

void RespawnC2SPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void RespawnC2SPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

void RespawnC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleRespawn(this);
}

int RespawnC2SPacket::GetEstimatedSize()
{
    return 1;
}
