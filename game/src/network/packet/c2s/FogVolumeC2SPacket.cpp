#include "FogVolumeC2SPacket.h"

#include "../../listener/PacketListener.h"

FogVolumeC2SPacket::FogVolumeC2SPacket()
{
    m_value   = {};
    m_removed = false;
}

FogVolumeC2SPacket::FogVolumeC2SPacket(const DynamicLights::FogVolume &value)
{
    m_value   = value;
    m_removed = false;
}

FogVolumeC2SPacket::FogVolumeC2SPacket(uint32_t removedId)
{
    m_value    = {};
    m_value.id = removedId;
    m_removed  = true;
}

FogVolumeC2SPacket::~FogVolumeC2SPacket()
{}

void FogVolumeC2SPacket::Read(DataInputStream *inputStream)
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

void FogVolumeC2SPacket::Write(DataOutputStream *outputStream)
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

bool FogVolumeC2SPacket::IsRemoved() const
{
    return m_removed;
}

const DynamicLights::FogVolume &FogVolumeC2SPacket::GetFogVolume() const
{
    return m_value;
}

ePACKET_ID FogVolumeC2SPacket::GetId()
{
    return ePacketId_FogVolumeC2S;
}

void FogVolumeC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleFogVolume(this);
}

int FogVolumeC2SPacket::GetEstimatedSize()
{
    return 64;
}
