#include "AtmosphereS2CPacket.h"

#include "../../listener/PacketListener.h"

AtmosphereS2CPacket::AtmosphereS2CPacket()
{
    m_airDensity = 0.0f;
    m_anisotropy = 0.0f;
}

AtmosphereS2CPacket::AtmosphereS2CPacket(float airDensity, float anisotropy)
{
    m_airDensity = airDensity;
    m_anisotropy = anisotropy;
}

AtmosphereS2CPacket::~AtmosphereS2CPacket()
{}

void AtmosphereS2CPacket::Read(DataInputStream *inputStream)
{
    m_airDensity = ReadClampedFloat(inputStream, 0.0f, 0.05f);
    m_anisotropy = ReadClampedFloat(inputStream, -0.9f, 0.9f);
}

void AtmosphereS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_airDensity);
    outputStream->WriteFloat(m_anisotropy);
}

float AtmosphereS2CPacket::GetAirDensity() const
{
    return m_airDensity;
}

float AtmosphereS2CPacket::GetAnisotropy() const
{
    return m_anisotropy;
}

ePACKET_ID AtmosphereS2CPacket::GetId()
{
    return ePacketId_AtmosphereS2C;
}

void AtmosphereS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleAtmosphere(this);
}

int AtmosphereS2CPacket::GetEstimatedSize()
{
    return 8;
}
