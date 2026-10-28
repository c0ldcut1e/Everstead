#include "TeamsS2CPacket.h"

#include <utility>

#include "../../../tag/NbtIo.h"
#include "../../listener/PacketListener.h"

TeamsS2CPacket::TeamsS2CPacket()
{}

TeamsS2CPacket::TeamsS2CPacket(std::unique_ptr<CompoundTag> tag)
{
    m_tag = std::move(tag);
}

TeamsS2CPacket::~TeamsS2CPacket()
{}

ePACKET_ID TeamsS2CPacket::GetId()
{
    return ePacketId_TeamsS2C;
}

void TeamsS2CPacket::Read(DataInputStream *inputStream)
{
    m_tag = NbtIo::Read(inputStream);
}

void TeamsS2CPacket::Write(DataOutputStream *outputStream)
{
    NbtIo::Write(m_tag.get(), outputStream);
}

void TeamsS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleTeams(this);
}

int TeamsS2CPacket::GetEstimatedSize()
{
    return 256;
}

const CompoundTag *TeamsS2CPacket::GetTag() const
{
    return m_tag.get();
}
