#include "ChangeDimensionS2CPacket.h"

#include "../../listener/PacketListener.h"

ChangeDimensionS2CPacket::ChangeDimensionS2CPacket()
{
    m_dimension = eDimensionId_Overworld;
    m_stability = 1.0f;
}

ChangeDimensionS2CPacket::ChangeDimensionS2CPacket(eDIMENSION_ID dimension, float stability)
{
    m_dimension = dimension;
    m_stability = stability;
}

ChangeDimensionS2CPacket::~ChangeDimensionS2CPacket()
{}

ePACKET_ID ChangeDimensionS2CPacket::GetId()
{
    return ePacketId_ChangeDimensionS2C;
}

void ChangeDimensionS2CPacket::Read(DataInputStream *inputStream)
{
    m_dimension = (eDIMENSION_ID) inputStream->ReadByte();
    m_stability = inputStream->ReadFloat();
}

void ChangeDimensionS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_dimension);
    outputStream->WriteFloat(m_stability);
}

void ChangeDimensionS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleChangeDimension(this);
}

int ChangeDimensionS2CPacket::GetEstimatedSize()
{
    return 5;
}

eDIMENSION_ID ChangeDimensionS2CPacket::GetDimension() const
{
    return m_dimension;
}

float ChangeDimensionS2CPacket::GetStability() const
{
    return m_stability;
}
