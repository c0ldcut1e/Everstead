#include "MultiBlockUpdateS2CPacket.h"

#include <algorithm>

#include "../../listener/PacketListener.h"

MultiBlockUpdateS2CPacket::MultiBlockUpdateS2CPacket()
{
    m_changes = {};
}

MultiBlockUpdateS2CPacket::MultiBlockUpdateS2CPacket(const std::vector<Change> &changes)
{
    m_changes = changes;
}

MultiBlockUpdateS2CPacket::~MultiBlockUpdateS2CPacket()
{}

ePACKET_ID MultiBlockUpdateS2CPacket::GetId()
{
    return ePacketId_MultiBlockUpdateS2C;
}

void MultiBlockUpdateS2CPacket::Read(DataInputStream *inputStream)
{
    uint32_t count = std::min(inputStream->ReadInt(), kMaxChanges);
    m_changes.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        m_changes[i].pos.x     = (int) inputStream->ReadInt();
        m_changes[i].pos.y     = (int) inputStream->ReadInt();
        m_changes[i].pos.z     = (int) inputStream->ReadInt();
        m_changes[i].blockId   = inputStream->ReadShort();
        m_changes[i].blockData = inputStream->ReadShort();
    }
}

void MultiBlockUpdateS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_changes.size());
    for (size_t i = 0; i < m_changes.size(); i++)
    {
        outputStream->WriteInt((uint32_t) m_changes[i].pos.x);
        outputStream->WriteInt((uint32_t) m_changes[i].pos.y);
        outputStream->WriteInt((uint32_t) m_changes[i].pos.z);
        outputStream->WriteShort(m_changes[i].blockId);
        outputStream->WriteShort(m_changes[i].blockData);
    }
}

void MultiBlockUpdateS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleMultiBlockUpdate(this);
}

int MultiBlockUpdateS2CPacket::GetEstimatedSize()
{
    return 4 + (int) m_changes.size() * 16;
}

const std::vector<MultiBlockUpdateS2CPacket::Change> &MultiBlockUpdateS2CPacket::GetChanges() const
{
    return m_changes;
}
