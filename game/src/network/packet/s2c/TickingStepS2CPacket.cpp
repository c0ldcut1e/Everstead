#include "TickingStepS2CPacket.h"

#include "../../listener/PacketListener.h"

TickingStepS2CPacket::TickingStepS2CPacket()
{
    m_steps = 0;
}

TickingStepS2CPacket::TickingStepS2CPacket(int steps)
{
    m_steps = steps;
}

TickingStepS2CPacket::~TickingStepS2CPacket()
{}

ePACKET_ID TickingStepS2CPacket::GetId()
{
    return ePacketId_TickingStepS2C;
}

void TickingStepS2CPacket::Read(DataInputStream *inputStream)
{
    m_steps = (int) inputStream->ReadInt();
}

void TickingStepS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_steps);
}

void TickingStepS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleTickingStep(this);
}

int TickingStepS2CPacket::GetEstimatedSize()
{
    return 4;
}

int TickingStepS2CPacket::GetSteps() const
{
    return m_steps;
}
