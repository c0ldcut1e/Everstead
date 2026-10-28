#include "TickingStateS2CPacket.h"

#include "../../listener/PacketListener.h"

TickingStateS2CPacket::TickingStateS2CPacket()
{
    m_tickRate = 20.0f;
    m_frozen   = false;
}

TickingStateS2CPacket::TickingStateS2CPacket(float tickRate, bool frozen)
{
    m_tickRate = tickRate;
    m_frozen   = frozen;
}

TickingStateS2CPacket::~TickingStateS2CPacket()
{}

ePACKET_ID TickingStateS2CPacket::GetId()
{
    return ePacketId_TickingStateS2C;
}

void TickingStateS2CPacket::Read(DataInputStream *inputStream)
{
    m_tickRate = inputStream->ReadFloat();
    m_frozen   = inputStream->ReadBool();
}

void TickingStateS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_tickRate);
    outputStream->WriteBool(m_frozen);
}

void TickingStateS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleTickingState(this);
}

int TickingStateS2CPacket::GetEstimatedSize()
{
    return 5;
}

float TickingStateS2CPacket::GetTickRate() const
{
    return m_tickRate;
}

bool TickingStateS2CPacket::IsFrozen() const
{
    return m_frozen;
}
