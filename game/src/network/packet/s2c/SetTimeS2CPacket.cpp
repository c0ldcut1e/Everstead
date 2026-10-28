#include "SetTimeS2CPacket.h"

#include "../../listener/PacketListener.h"

SetTimeS2CPacket::SetTimeS2CPacket()
{
    m_ticks         = 0;
    m_daylightCycle = true;
}

SetTimeS2CPacket::SetTimeS2CPacket(int64_t ticks, bool daylightCycle)
{
    m_ticks         = ticks;
    m_daylightCycle = daylightCycle;
}

SetTimeS2CPacket::~SetTimeS2CPacket()
{}

ePACKET_ID SetTimeS2CPacket::GetId()
{
    return ePacketId_SetTimeS2C;
}

void SetTimeS2CPacket::Read(DataInputStream *inputStream)
{
    m_ticks         = (int64_t) inputStream->ReadLong();
    m_daylightCycle = inputStream->ReadBool();
}

void SetTimeS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteLong((uint64_t) m_ticks);
    outputStream->WriteBool(m_daylightCycle);
}

void SetTimeS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetTime(this);
}

int SetTimeS2CPacket::GetEstimatedSize()
{
    return 10;
}

int64_t SetTimeS2CPacket::GetTicks() const
{
    return m_ticks;
}

bool SetTimeS2CPacket::IsDaylightCycle() const
{
    return m_daylightCycle;
}
