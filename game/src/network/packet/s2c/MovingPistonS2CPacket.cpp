#include "MovingPistonS2CPacket.h"

#include <utility>

#include "../../../tag/NbtIo.h"
#include "../../listener/PacketListener.h"

MovingPistonS2CPacket::MovingPistonS2CPacket()
{
    m_pos = BlockPos(0, 0, 0);
}

MovingPistonS2CPacket::MovingPistonS2CPacket(const BlockPos &pos, std::unique_ptr<CompoundTag> tag)
{
    m_pos = pos;
    m_tag = std::move(tag);
}

MovingPistonS2CPacket::~MovingPistonS2CPacket()
{}

ePACKET_ID MovingPistonS2CPacket::GetId()
{
    return ePacketId_MovingPistonS2C;
}

void MovingPistonS2CPacket::Read(DataInputStream *inputStream)
{
    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);
    m_tag    = NbtIo::Read(inputStream);
}

void MovingPistonS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    NbtIo::Write(m_tag.get(), outputStream);
}

void MovingPistonS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleMovingPiston(this);
}

int MovingPistonS2CPacket::GetEstimatedSize()
{
    return 64;
}

const BlockPos &MovingPistonS2CPacket::GetPos() const
{
    return m_pos;
}

const CompoundTag *MovingPistonS2CPacket::GetTag() const
{
    return m_tag.get();
}
