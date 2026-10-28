#include "AttackC2SPacket.h"

#include "../../listener/PacketListener.h"

AttackC2SPacket::AttackC2SPacket()
{
    m_entityId = 0;
}

AttackC2SPacket::AttackC2SPacket(int entityId)
{
    m_entityId = entityId;
}

AttackC2SPacket::~AttackC2SPacket()
{}

ePACKET_ID AttackC2SPacket::GetId()
{
    return ePacketId_AttackC2S;
}

void AttackC2SPacket::Read(DataInputStream *inputStream)
{
    m_entityId = (int) inputStream->ReadInt();
}

void AttackC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteInt((uint32_t) m_entityId);
}

void AttackC2SPacket::Handle(PacketListener *listener)
{
    listener->HandleAttack(this);
}

int AttackC2SPacket::GetEstimatedSize()
{
    return 5;
}

int AttackC2SPacket::GetEntityId() const
{
    return m_entityId;
}
