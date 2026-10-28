#include "CustomPayloadS2CPacket.h"

#include "../../listener/PacketListener.h"

const int CustomPayloadS2CPacket::s_maxPayloadSize = 32767;

CustomPayloadS2CPacket::CustomPayloadS2CPacket()
{
    m_channel = ResourceLocation();
    m_payload = {};
}

CustomPayloadS2CPacket::CustomPayloadS2CPacket(const ResourceLocation &channel, const std::vector<uint8_t> &payload)
{
    m_channel = channel;
    m_payload = payload;
}

CustomPayloadS2CPacket::~CustomPayloadS2CPacket()
{}

ePACKET_ID CustomPayloadS2CPacket::GetId()
{
    return ePacketId_CustomPayloadS2C;
}

void CustomPayloadS2CPacket::Read(DataInputStream *inputStream)
{
    std::string channelNamespace = inputStream->ReadString();
    std::string channelPath      = inputStream->ReadString();
    m_channel                    = ResourceLocation(channelNamespace, channelPath);
    m_payload                    = inputStream->ReadBytes();
    if ((int) m_payload.size() > s_maxPayloadSize)
    {
        m_payload.clear();
    }
}

void CustomPayloadS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_channel.GetNamespace());
    outputStream->WriteString(m_channel.GetPath());
    outputStream->WriteBytes(m_payload);
}

void CustomPayloadS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleCustomPayload(this);
}

int CustomPayloadS2CPacket::GetEstimatedSize()
{
    return (int) (m_channel.GetNamespace().size() + m_channel.GetPath().size() + m_payload.size() + 16);
}

const ResourceLocation &CustomPayloadS2CPacket::GetChannel() const
{
    return m_channel;
}

const std::vector<uint8_t> &CustomPayloadS2CPacket::GetPayload() const
{
    return m_payload;
}
