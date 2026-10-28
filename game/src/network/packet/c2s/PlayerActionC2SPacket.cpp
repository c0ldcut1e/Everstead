#include "PlayerActionC2SPacket.h"

#include "../../listener/PacketListener.h"

PlayerActionC2SPacket::PlayerActionC2SPacket()
{
    m_action            = ePlayerAction_Break;
    m_pos               = BlockPos(0, 0, 0);
    m_placedAgainstFace = 0;
    m_data              = 0;
}

PlayerActionC2SPacket::PlayerActionC2SPacket(ePLAYER_ACTION action, const BlockPos &pos, uint8_t placedAgainstFace, uint16_t data)
{
    m_action            = action;
    m_pos               = pos;
    m_placedAgainstFace = placedAgainstFace;
    m_data              = data;
}

PlayerActionC2SPacket::~PlayerActionC2SPacket()
{}

ePACKET_ID PlayerActionC2SPacket::GetId()
{
    return ePacketId_PlayerActionC2S;
}

void PlayerActionC2SPacket::Read(DataInputStream *inputStream)
{
    m_action = (ePLAYER_ACTION) inputStream->ReadByte();

    int posX = (int) inputStream->ReadInt();
    int posY = (int) inputStream->ReadInt();
    int posZ = (int) inputStream->ReadInt();
    m_pos    = BlockPos(posX, posY, posZ);

    m_placedAgainstFace = inputStream->ReadByte();
    m_data              = (uint16_t) inputStream->ReadShort();
}

void PlayerActionC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_action);
    outputStream->WriteInt((uint32_t) m_pos.x);
    outputStream->WriteInt((uint32_t) m_pos.y);
    outputStream->WriteInt((uint32_t) m_pos.z);
    outputStream->WriteByte(m_placedAgainstFace);
    outputStream->WriteShort(m_data);
}

void PlayerActionC2SPacket::Handle(PacketListener *listener)
{
    listener->HandlePlayerAction(this);
}

int PlayerActionC2SPacket::GetEstimatedSize()
{
    return 17;
}

ePLAYER_ACTION PlayerActionC2SPacket::GetAction() const
{
    return m_action;
}

const BlockPos &PlayerActionC2SPacket::GetPos() const
{
    return m_pos;
}

uint8_t PlayerActionC2SPacket::GetPlacedAgainstFace() const
{
    return m_placedAgainstFace;
}

uint16_t PlayerActionC2SPacket::GetData() const
{
    return m_data;
}
