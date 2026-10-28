#include "DynamicLightS2CPacket.h"

#include "../../listener/PacketListener.h"

DynamicLightS2CPacket::DynamicLightS2CPacket()
{
    m_value   = {};
    m_removed = false;
}

DynamicLightS2CPacket::DynamicLightS2CPacket(const DynamicLights::Light &value)
{
    m_value   = value;
    m_removed = false;
}

DynamicLightS2CPacket::DynamicLightS2CPacket(uint32_t removedId)
{
    m_value    = {};
    m_value.id = removedId;
    m_removed  = true;
}

DynamicLightS2CPacket::~DynamicLightS2CPacket()
{}

void DynamicLightS2CPacket::Read(DataInputStream *inputStream)
{
    m_removed = inputStream->ReadBool();
    if (m_removed)
    {
        m_value    = {};
        m_value.id = inputStream->ReadInt();
    }
    else
    {
        m_value = ReadDynamicLight(inputStream);
    }
}

void DynamicLightS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteBool(m_removed);
    if (m_removed)
    {
        outputStream->WriteInt(m_value.id);
    }
    else
    {
        WriteDynamicLight(outputStream, m_value);
    }
}

bool DynamicLightS2CPacket::IsRemoved() const
{
    return m_removed;
}

const DynamicLights::Light &DynamicLightS2CPacket::GetLight() const
{
    return m_value;
}

ePACKET_ID DynamicLightS2CPacket::GetId()
{
    return ePacketId_DynamicLightS2C;
}

void DynamicLightS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleDynamicLight(this);
}

int DynamicLightS2CPacket::GetEstimatedSize()
{
    return 64;
}
