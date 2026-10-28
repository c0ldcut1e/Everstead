#include "SetWeatherS2CPacket.h"

#include <algorithm>
#include <cmath>

#include "../../listener/PacketListener.h"

SetWeatherS2CPacket::SetWeatherS2CPacket()
{
    m_raining      = false;
    m_thundering   = false;
    m_rainLevel    = 0.0f;
    m_thunderLevel = 0.0f;
}

SetWeatherS2CPacket::SetWeatherS2CPacket(bool raining, bool thundering, float rainLevel, float thunderLevel)
{
    m_raining      = raining;
    m_thundering   = thundering;
    m_rainLevel    = rainLevel;
    m_thunderLevel = thunderLevel;
}

SetWeatherS2CPacket::~SetWeatherS2CPacket()
{}

ePACKET_ID SetWeatherS2CPacket::GetId()
{
    return ePacketId_SetWeatherS2C;
}

void SetWeatherS2CPacket::Read(DataInputStream *inputStream)
{
    m_raining      = inputStream->ReadBool();
    m_thundering   = inputStream->ReadBool();
    m_rainLevel    = inputStream->ReadFloat();
    m_thunderLevel = inputStream->ReadFloat();
    m_rainLevel    = std::isfinite(m_rainLevel) ? std::clamp(m_rainLevel, 0.0f, 1.0f) : 0.0f;
    m_thunderLevel = std::isfinite(m_thunderLevel) ? std::clamp(m_thunderLevel, 0.0f, 1.0f) : 0.0f;
}

void SetWeatherS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteBool(m_raining);
    outputStream->WriteBool(m_thundering);
    outputStream->WriteFloat(m_rainLevel);
    outputStream->WriteFloat(m_thunderLevel);
}

void SetWeatherS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetWeather(this);
}

int SetWeatherS2CPacket::GetEstimatedSize()
{
    return 10;
}

bool SetWeatherS2CPacket::IsRaining() const
{
    return m_raining;
}

bool SetWeatherS2CPacket::IsThundering() const
{
    return m_thundering;
}

float SetWeatherS2CPacket::GetRainLevel() const
{
    return m_rainLevel;
}

float SetWeatherS2CPacket::GetThunderLevel() const
{
    return m_thunderLevel;
}
