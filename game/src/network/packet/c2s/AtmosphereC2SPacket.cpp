#include "AtmosphereC2SPacket.h"

#include "../../listener/PacketListener.h"

AtmosphereC2SPacket::AtmosphereC2SPacket()
{
    m_airDensity = 0.0f;
    m_anisotropy = 0.0f;
}

AtmosphereC2SPacket::AtmosphereC2SPacket(float airDensity, float anisotropy)
{
    m_airDensity = airDensity;
    m_anisotropy = anisotropy;
}

AtmosphereC2SPacket::~AtmosphereC2SPacket()
{}

void AtmosphereC2SPacket::Read(DataInputStream *inputStream)
{
    m_airDensity = ReadClampedFloat(inputStream, 0.0f, 0.05f);
    m_anisotropy = ReadClampedFloat(inputStream, -0.9f, 0.9f);
}

void AtmosphereC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_airDensity);
    outputStream->WriteFloat(m_anisotropy);
}

float AtmosphereC2SPacket::GetAirDensity() const
{
    return m_airDensity;
}

float AtmosphereC2SPacket::GetAnisotropy() const
{
    return m_anisotropy;
}

ePACKET_ID AtmosphereC2SPacket::GetId()
{
    return ePacketId_AtmosphereC2S;
}

void AtmosphereC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleAtmosphere(this);
}

int AtmosphereC2SPacket::GetEstimatedSize()
{
    return 8;
}
