#include "SwingC2SPacket.h"

#include "../../listener/PacketListener.h"

SwingC2SPacket::SwingC2SPacket()
{
}

SwingC2SPacket::~SwingC2SPacket()
{
}

ePACKET_ID SwingC2SPacket::GetId()
{
    return ePacketId_SwingC2S;
}

void SwingC2SPacket::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void SwingC2SPacket::Write(DataOutputStream *outputStream)
{
    (void) outputStream;
}

void SwingC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleSwing(this);
}

int SwingC2SPacket::GetEstimatedSize()
{
    return 1;
}
