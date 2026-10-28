#include "LevelReadyS2CPacket.h"

#include "../../listener/PacketListener.h"

LevelReadyS2CPacket::LevelReadyS2CPacket()
{
}

LevelReadyS2CPacket::~LevelReadyS2CPacket()
{
}

ePACKET_ID LevelReadyS2CPacket::GetId()
{
    return ePacketId_LevelReadyS2C;
}

void LevelReadyS2CPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void LevelReadyS2CPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

void LevelReadyS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleLevelReady(this);
}
