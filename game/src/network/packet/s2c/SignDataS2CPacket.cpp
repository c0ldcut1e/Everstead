#include "SignDataS2CPacket.h"

#include <utility>

#include "../../../tag/NbtIo.h"
#include "../../listener/PacketListener.h"

SignDataS2CPacket::SignDataS2CPacket()
{
    m_pos = BlockPos(0, 0, 0);
}

SignDataS2CPacket::SignDataS2CPacket(const BlockPos &pos, std::unique_ptr<CompoundTag> tag)
{
    m_pos = pos;
    m_tag = std::move(tag);
}

SignDataS2CPacket::~SignDataS2CPacket()
{}

ePACKET_ID SignDataS2CPacket::GetId()
{
    return ePacketId_SignDataS2C;
}

void SignDataS2CPacket::Read(DataInputStream *inputStream)
{
    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);
    m_tag    = NbtIo::Read(inputStream);
}

void SignDataS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    NbtIo::Write(m_tag.get(), outputStream);
}

void SignDataS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSignData(this);
}

int SignDataS2CPacket::GetEstimatedSize()
{
    return 256;
}

const BlockPos &SignDataS2CPacket::GetPos() const
{
    return m_pos;
}

const CompoundTag *SignDataS2CPacket::GetTag() const
{
    return m_tag.get();
}
