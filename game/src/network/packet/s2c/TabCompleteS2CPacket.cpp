#include "TabCompleteS2CPacket.h"

#include <algorithm>

#include "../../listener/PacketListener.h"

TabCompleteS2CPacket::TabCompleteS2CPacket()
{
    m_completions = {};
}

TabCompleteS2CPacket::TabCompleteS2CPacket(const std::vector<std::string> &completions)
{
    m_completions = completions;
}

TabCompleteS2CPacket::~TabCompleteS2CPacket()
{}

ePACKET_ID TabCompleteS2CPacket::GetId()
{
    return ePacketId_TabCompleteS2C;
}

void TabCompleteS2CPacket::Read(DataInputStream *inputStream)
{
    uint16_t count = std::min(inputStream->ReadShort(), kMaxCompletions);
    m_completions.clear();
    for (uint16_t i = 0; i < count; i++)
    {
        m_completions.push_back(inputStream->ReadString());
    }
}

void TabCompleteS2CPacket::Write(DataOutputStream *outputStream)
{
    uint16_t count = (uint16_t) std::min(m_completions.size(), (size_t) kMaxCompletions);
    outputStream->WriteShort(count);
    for (uint16_t i = 0; i < count; i++)
    {
        outputStream->WriteString(m_completions[i]);
    }
}

void TabCompleteS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleTabComplete(this);
}

int TabCompleteS2CPacket::GetEstimatedSize()
{
    int size = 2;
    for (size_t i = 0; i < m_completions.size(); i++)
    {
        size += 4 + (int) m_completions[i].size();
    }

    return size;
}

const std::vector<std::string> &TabCompleteS2CPacket::GetCompletions() const
{
    return m_completions;
}
