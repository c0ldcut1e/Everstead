#include "SetBoatDataS2CPacket.h"

#include "../../listener/PacketListener.h"

SetBoatDataS2CPacket::SetBoatDataS2CPacket()
{
    m_entityId    = 0;
    m_leftPaddle  = false;
    m_rightPaddle = false;
}

SetBoatDataS2CPacket::SetBoatDataS2CPacket(int entityId, bool leftPaddle, bool rightPaddle)
{
    m_entityId    = entityId;
    m_leftPaddle  = leftPaddle;
    m_rightPaddle = rightPaddle;
}

SetBoatDataS2CPacket::~SetBoatDataS2CPacket()
{}

ePACKET_ID SetBoatDataS2CPacket::GetId()
{
    return ePacketId_SetBoatDataS2C;
}

void SetBoatDataS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId    = (int) inputStream->ReadInt();
    m_leftPaddle  = inputStream->ReadBool();
    m_rightPaddle = inputStream->ReadBool();
}

void SetBoatDataS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteBool(m_leftPaddle);
    outputStream->WriteBool(m_rightPaddle);
}

void SetBoatDataS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetBoatData(this);
}

int SetBoatDataS2CPacket::GetEstimatedSize()
{
    return 6;
}

int SetBoatDataS2CPacket::GetEntityId() const
{
    return m_entityId;
}

bool SetBoatDataS2CPacket::GetLeftPaddle() const
{
    return m_leftPaddle;
}

bool SetBoatDataS2CPacket::GetRightPaddle() const
{
    return m_rightPaddle;
}
