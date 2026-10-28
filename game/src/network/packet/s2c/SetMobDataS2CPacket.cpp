#include "SetMobDataS2CPacket.h"

#include "../../listener/PacketListener.h"

SetMobDataS2CPacket::SetMobDataS2CPacket()
{
    m_entityId = 0;
    m_data     = MobData();
}

SetMobDataS2CPacket::SetMobDataS2CPacket(int entityId, const MobData &data)
{
    m_entityId = entityId;
    m_data     = data;
}

SetMobDataS2CPacket::~SetMobDataS2CPacket()
{}

ePACKET_ID SetMobDataS2CPacket::GetId()
{
    return ePacketId_SetMobDataS2C;
}

void SetMobDataS2CPacket::Read(DataInputStream *inputStream)
{
    m_entityId           = (int) inputStream->ReadInt();
    m_data.health        = (int) inputStream->ReadInt();
    m_data.flags         = inputStream->ReadByte();
    m_data.baby          = inputStream->ReadBool();
    m_data.inLove        = inputStream->ReadBool();
    m_data.variant       = inputStream->ReadByte();
    m_data.leashHolderId = (int) inputStream->ReadInt();
}

void SetMobDataS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
    outputStream->WriteInt((uint32_t) m_data.health);
    outputStream->WriteByte(m_data.flags);
    outputStream->WriteBool(m_data.baby);
    outputStream->WriteBool(m_data.inLove);
    outputStream->WriteByte(m_data.variant);
    outputStream->WriteInt((uint32_t) m_data.leashHolderId);
}

void SetMobDataS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetMobData(this);
}

int SetMobDataS2CPacket::GetEstimatedSize()
{
    return 16;
}

int SetMobDataS2CPacket::GetEntityId() const
{
    return m_entityId;
}

const MobData &SetMobDataS2CPacket::GetData() const
{
    return m_data;
}
