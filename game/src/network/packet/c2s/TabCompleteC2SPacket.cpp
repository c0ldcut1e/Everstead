#include "TabCompleteC2SPacket.h"

#include "../../listener/PacketListener.h"

TabCompleteC2SPacket::TabCompleteC2SPacket()
{
    m_text      = "";
    m_hasTarget = false;
    m_target    = BlockPos(0, 0, 0);
}

TabCompleteC2SPacket::TabCompleteC2SPacket(const std::string &text, bool hasTarget, const BlockPos &target)
{
    m_text      = text;
    m_hasTarget = hasTarget;
    m_target    = target;
}

TabCompleteC2SPacket::~TabCompleteC2SPacket()
{}

ePACKET_ID TabCompleteC2SPacket::GetId()
{
    return ePacketId_TabCompleteC2S;
}

void TabCompleteC2SPacket::Read(DataInputStream *inputStream)
{
    m_text      = inputStream->ReadString();
    m_hasTarget = inputStream->ReadBool();
    if (m_hasTarget)
    {
        m_target.x = (int) inputStream->ReadInt();
        m_target.y = (int) inputStream->ReadInt();
        m_target.z = (int) inputStream->ReadInt();
    }
}

void TabCompleteC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteString(m_text);
    outputStream->WriteBool(m_hasTarget);
    if (m_hasTarget)
    {
        outputStream->WriteInt((uint32_t) m_target.x);
        outputStream->WriteInt((uint32_t) m_target.y);
        outputStream->WriteInt((uint32_t) m_target.z);
    }
}

void TabCompleteC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleTabComplete(this);
}

int TabCompleteC2SPacket::GetEstimatedSize()
{
    return 17 + (int) m_text.size();
}

const std::string &TabCompleteC2SPacket::GetText() const
{
    return m_text;
}

bool TabCompleteC2SPacket::HasTarget() const
{
    return m_hasTarget;
}

const BlockPos &TabCompleteC2SPacket::GetTarget() const
{
    return m_target;
}
