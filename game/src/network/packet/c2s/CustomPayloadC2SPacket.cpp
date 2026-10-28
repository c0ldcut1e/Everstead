#include "CustomPayloadC2SPacket.h"

#include "../../listener/PacketListener.h"

const int CustomPayloadC2SPacket::s_maxPayloadSize = 32767;

CustomPayloadC2SPacket::CustomPayloadC2SPacket()
{
    m_channel = ResourceLocation();
    m_payload = {};
}

CustomPayloadC2SPacket::CustomPayloadC2SPacket(const ResourceLocation &channel, const std::vector<uint8_t> &payload)
{
    m_channel = channel;
    m_payload = payload;
}

CustomPayloadC2SPacket::~CustomPayloadC2SPacket()
{}

ePACKET_ID CustomPayloadC2SPacket::GetId()
{
    return ePacketId_CustomPayloadC2S;
}

void CustomPayloadC2SPacket::Read(DataInputStream *inputStream)
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

void CustomPayloadC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_channel.GetNamespace());
    outputStream->WriteString(m_channel.GetPath());
    outputStream->WriteBytes(m_payload);
}

void CustomPayloadC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleCustomPayload(this);
}

int CustomPayloadC2SPacket::GetEstimatedSize()
{
    return (int) (m_channel.GetNamespace().size() + m_channel.GetPath().size() + m_payload.size() + 16);
}

const ResourceLocation &CustomPayloadC2SPacket::GetChannel() const
{
    return m_channel;
}

const std::vector<uint8_t> &CustomPayloadC2SPacket::GetPayload() const
{
    return m_payload;
}
