#include "SetHealthS2CPacket.h"

#include "../../listener/PacketListener.h"

SetHealthS2CPacket::SetHealthS2CPacket()
{
    m_health          = 0;
    m_foodLevel       = 0;
    m_saturationLevel = 0.0f;
}

SetHealthS2CPacket::SetHealthS2CPacket(int health, int foodLevel, float saturationLevel)
{
    m_health          = health;
    m_foodLevel       = foodLevel;
    m_saturationLevel = saturationLevel;
}

SetHealthS2CPacket::~SetHealthS2CPacket()
{}

ePACKET_ID SetHealthS2CPacket::GetId()
{
    return ePacketId_SetHealthS2C;
}

void SetHealthS2CPacket::Read(DataInputStream *inputStream)
{
    m_health          = inputStream->ReadByte();
    m_foodLevel       = inputStream->ReadShort();
    m_saturationLevel = inputStream->ReadFloat();
}

void SetHealthS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_health);
    outputStream->WriteShort((uint16_t) m_foodLevel);
    outputStream->WriteFloat(m_saturationLevel);
}

void SetHealthS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetHealth(this);
}

int SetHealthS2CPacket::GetEstimatedSize()
{
    return 7;
}

int SetHealthS2CPacket::GetHealth() const
{
    return m_health;
}

int SetHealthS2CPacket::GetFoodLevel() const
{
    return m_foodLevel;
}

float SetHealthS2CPacket::GetSaturationLevel() const
{
    return m_saturationLevel;
}
