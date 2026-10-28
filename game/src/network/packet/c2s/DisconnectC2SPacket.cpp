#include "DisconnectC2SPacket.h"

#include "../../listener/PacketListener.h"

DisconnectC2SPacket::DisconnectC2SPacket()
{
    m_reason = eDisconnectReason_None;
}

DisconnectC2SPacket::DisconnectC2SPacket(eDISCONNECT_REASON reason)
{
    m_reason = reason;
}

DisconnectC2SPacket::~DisconnectC2SPacket()
{
}

ePACKET_ID DisconnectC2SPacket::GetId()
{
    return ePacketId_DisconnectC2S;
}

void DisconnectC2SPacket::Read(DataInputStream *inputStream)
{
    m_reason = (eDISCONNECT_REASON) inputStream->ReadByte();
}

void DisconnectC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_reason);
}

void DisconnectC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleDisconnect(this);
}

int DisconnectC2SPacket::GetEstimatedSize()
{
    return 1;
}

eDISCONNECT_REASON DisconnectC2SPacket::GetReason() const
{
    return m_reason;
}
