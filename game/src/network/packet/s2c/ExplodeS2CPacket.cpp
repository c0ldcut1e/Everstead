#include "ExplodeS2CPacket.h"

#include <cmath>

#include "../../listener/PacketListener.h"

ExplodeS2CPacket::ExplodeS2CPacket()
{
    m_radius = 0.0f;
    m_type   = eExplosionType_Normal;
}

ExplodeS2CPacket::ExplodeS2CPacket(const glm::dvec3 &center, float radius, const std::vector<BlockPos> &blocks, const glm::dvec3 &knockback,
                                   eEXPLOSION_TYPE type)
{
    m_center    = center;
    m_radius    = radius;
    m_blocks    = blocks;
    m_knockback = knockback;
    m_type      = type;
}

ExplodeS2CPacket::~ExplodeS2CPacket()
{}

ePACKET_ID ExplodeS2CPacket::GetId()
{
    return ePacketId_ExplodeS2C;
}

// Blocks are sent as signed byte offsets from the block holding the center
void ExplodeS2CPacket::Read(DataInputStream *inputStream)
{
    m_center.x = inputStream->ReadDouble();
    m_center.y = inputStream->ReadDouble();
    m_center.z = inputStream->ReadDouble();
    m_radius   = inputStream->ReadFloat();
    BlockPos origin((int) floor(m_center.x), (int) floor(m_center.y), (int) floor(m_center.z));
    int count = (int) inputStream->ReadInt();
    m_blocks.resize(count);
    for (int i = 0; i < count; i++)
    {
        int x       = (int8_t) inputStream->ReadByte();
        int y       = (int8_t) inputStream->ReadByte();
        int z       = (int8_t) inputStream->ReadByte();
        m_blocks[i] = origin + BlockPos(x, y, z);
    }

    m_knockback.x = inputStream->ReadFloat();
    m_knockback.y = inputStream->ReadFloat();
    m_knockback.z = inputStream->ReadFloat();
    m_type        = (eEXPLOSION_TYPE) inputStream->ReadByte();
}

void ExplodeS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteDouble(m_center.x);
    outputStream->WriteDouble(m_center.y);
    outputStream->WriteDouble(m_center.z);
    outputStream->WriteFloat(m_radius);
    BlockPos origin((int) floor(m_center.x), (int) floor(m_center.y), (int) floor(m_center.z));
    outputStream->WriteInt((uint32_t) m_blocks.size());
    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        BlockPos offset = m_blocks[i] - origin;
        outputStream->WriteByte((uint8_t) offset.x);
        outputStream->WriteByte((uint8_t) offset.y);
        outputStream->WriteByte((uint8_t) offset.z);
    }

    outputStream->WriteFloat((float) m_knockback.x);
    outputStream->WriteFloat((float) m_knockback.y);
    outputStream->WriteFloat((float) m_knockback.z);
    outputStream->WriteByte((uint8_t) m_type);
}

void ExplodeS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleExplode(this);
}

int ExplodeS2CPacket::GetEstimatedSize()
{
    return 41 + (int) m_blocks.size() * 3;
}

const glm::dvec3 &ExplodeS2CPacket::GetCenter() const
{
    return m_center;
}

float ExplodeS2CPacket::GetRadius() const
{
    return m_radius;
}

const std::vector<BlockPos> &ExplodeS2CPacket::GetBlocks() const
{
    return m_blocks;
}

const glm::dvec3 &ExplodeS2CPacket::GetKnockback() const
{
    return m_knockback;
}

eEXPLOSION_TYPE ExplodeS2CPacket::GetType() const
{
    return m_type;
}
