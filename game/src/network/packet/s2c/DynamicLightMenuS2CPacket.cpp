#include "DynamicLightMenuS2CPacket.h"

#include "../../listener/PacketListener.h"

DynamicLightMenuS2CPacket::DynamicLightMenuS2CPacket()
{}

DynamicLightMenuS2CPacket::~DynamicLightMenuS2CPacket()
{}

void DynamicLightMenuS2CPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void DynamicLightMenuS2CPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

ePACKET_ID DynamicLightMenuS2CPacket::GetId()
{
    return ePacketId_DynamicLightMenuS2C;
}

void DynamicLightMenuS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleDynamicLightMenu(this);
}

int DynamicLightMenuS2CPacket::GetEstimatedSize()
{
    return 1;
}
