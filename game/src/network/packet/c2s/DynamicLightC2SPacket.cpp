#include "DynamicLightC2SPacket.h"

#include "../../listener/PacketListener.h"

DynamicLightC2SPacket::DynamicLightC2SPacket()
{
    m_value   = {};
    m_removed = false;
}

DynamicLightC2SPacket::DynamicLightC2SPacket(const DynamicLights::Light &value)
{
    m_value   = value;
    m_removed = false;
}

DynamicLightC2SPacket::DynamicLightC2SPacket(uint32_t removedId)
{
    m_value    = {};
    m_value.id = removedId;
    m_removed  = true;
}

DynamicLightC2SPacket::~DynamicLightC2SPacket()
{}

void DynamicLightC2SPacket::Read(DataInputStream *inputStream)
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

void DynamicLightC2SPacket::Write(DataOutputStream *outputStream)
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

bool DynamicLightC2SPacket::IsRemoved() const
{
    return m_removed;
}

const DynamicLights::Light &DynamicLightC2SPacket::GetLight() const
{
    return m_value;
}

ePACKET_ID DynamicLightC2SPacket::GetId()
{
    return ePacketId_DynamicLightC2S;
}

void DynamicLightC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleDynamicLight(this);
}

int DynamicLightC2SPacket::GetEstimatedSize()
{
    return 64;
}
