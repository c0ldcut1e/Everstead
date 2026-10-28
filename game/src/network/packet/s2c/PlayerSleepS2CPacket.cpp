#include "PlayerSleepS2CPacket.h"

#include "../../listener/PacketListener.h"

PlayerSleepS2CPacket::PlayerSleepS2CPacket()
{
    m_playerIndex = 0;
    m_sleeping    = false;
    m_pos         = BlockPos(0, 0, 0);
}

PlayerSleepS2CPacket::PlayerSleepS2CPacket(uint8_t playerIndex, bool sleeping, const BlockPos &pos)
{
    m_playerIndex = playerIndex;
    m_sleeping    = sleeping;
    m_pos         = pos;
}

PlayerSleepS2CPacket::~PlayerSleepS2CPacket()
{}

ePACKET_ID PlayerSleepS2CPacket::GetId()
{
    return ePacketId_PlayerSleepS2C;
}

void PlayerSleepS2CPacket::Read(DataInputStream *inputStream)
{
    m_playerIndex = inputStream->ReadByte();
    m_sleeping    = inputStream->ReadByte() != 0;
    int posX      = (int) inputStream->ReadInt();
    int posY      = (int) inputStream->ReadInt();
    int posZ      = (int) inputStream->ReadInt();
    m_pos         = BlockPos(posX, posY, posZ);
}

void PlayerSleepS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte(m_playerIndex);
    outputStream->WriteByte(m_sleeping ? 1 : 0);
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
}

void PlayerSleepS2CPacket::Handle(PacketListener *listener)
{
    listener->HandlePlayerSleep(this);
}

int PlayerSleepS2CPacket::GetEstimatedSize()
{
    return 14;
}

uint8_t PlayerSleepS2CPacket::GetPlayerIndex() const
{
    return m_playerIndex;
}

bool PlayerSleepS2CPacket::IsSleeping() const
{
    return m_sleeping;
}

const BlockPos &PlayerSleepS2CPacket::GetPos() const
{
    return m_pos;
}
