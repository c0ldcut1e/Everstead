#include "FogVolumeS2CPacket.h"

#include "../../listener/PacketListener.h"

FogVolumeS2CPacket::FogVolumeS2CPacket()
{
    m_value   = {};
    m_removed = false;
}

FogVolumeS2CPacket::FogVolumeS2CPacket(const DynamicLights::FogVolume &value)
{
    m_value   = value;
    m_removed = false;
}

FogVolumeS2CPacket::FogVolumeS2CPacket(uint32_t removedId)
{
    m_value    = {};
    m_value.id = removedId;
    m_removed  = true;
}

FogVolumeS2CPacket::~FogVolumeS2CPacket()
{}

void FogVolumeS2CPacket::Read(DataInputStream *inputStream)
{
    m_removed = inputStream->ReadBool();
    if (m_removed)
    {
        m_value    = {};
        m_value.id = inputStream->ReadInt();
    }
    else
    {
        m_value = ReadFogVolume(inputStream);
    }
}

void FogVolumeS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteBool(m_removed);
    if (m_removed)
    {
        outputStream->WriteInt(m_value.id);
    }
    else
    {
        WriteFogVolume(outputStream, m_value);
    }
}

bool FogVolumeS2CPacket::IsRemoved() const
{
    return m_removed;
}

const DynamicLights::FogVolume &FogVolumeS2CPacket::GetFogVolume() const
{
    return m_value;
}

ePACKET_ID FogVolumeS2CPacket::GetId()
{
    return ePacketId_FogVolumeS2C;
}

void FogVolumeS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleFogVolume(this);
}

int FogVolumeS2CPacket::GetEstimatedSize()
{
    return 64;
}
