#include "PlayerInputC2SPacket.h"

#include "../../listener/PacketListener.h"

PlayerInputC2SPacket::PlayerInputC2SPacket()
{
    m_forward = 0.0f;
}

PlayerInputC2SPacket::PlayerInputC2SPacket(float forward)
{
    m_forward = forward;
}

PlayerInputC2SPacket::~PlayerInputC2SPacket()
{}

ePACKET_ID PlayerInputC2SPacket::GetId()
{
    return ePacketId_PlayerInputC2S;
}

void PlayerInputC2SPacket::Read(DataInputStream *inputStream)
{
    m_forward = inputStream->ReadFloat();
}

void PlayerInputC2SPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteFloat(m_forward);
}

void PlayerInputC2SPacket::Handle(PacketListener *listener)
{
    listener->HandlePlayerInput(this);
}

int PlayerInputC2SPacket::GetEstimatedSize()
{
    return 4;
}

float PlayerInputC2SPacket::GetForward() const
{
    return m_forward;
}
