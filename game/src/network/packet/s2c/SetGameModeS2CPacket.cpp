#include "SetGameModeS2CPacket.h"

#include "../../listener/PacketListener.h"

SetGameModeS2CPacket::SetGameModeS2CPacket()
{
    m_gameType = eGameType_Survival;
    m_flying   = false;
}

SetGameModeS2CPacket::SetGameModeS2CPacket(eGAME_TYPE gameType, bool flying)
{
    m_gameType = gameType;
    m_flying   = flying;
}

SetGameModeS2CPacket::~SetGameModeS2CPacket()
{}

ePACKET_ID SetGameModeS2CPacket::GetId()
{
    return ePacketId_SetGameModeS2C;
}

void SetGameModeS2CPacket::Read(DataInputStream *inputStream)
{
    int gameType = inputStream->ReadByte();
    m_gameType   = GameType::IsValid(gameType) ? (eGAME_TYPE) gameType : eGameType_Survival;
    m_flying     = inputStream->ReadByte() != 0;
}

void SetGameModeS2CPacket::Write(DataOutputStream *outputStream)
{
    outputStream->WriteByte((uint8_t) m_gameType);
    outputStream->WriteByte((uint8_t) m_flying);
}

void SetGameModeS2CPacket::Handle(PacketListener *listener)
{
    listener->HandleSetGameMode(this);
}

int SetGameModeS2CPacket::GetEstimatedSize()
{
    return 2;
}

eGAME_TYPE SetGameModeS2CPacket::GetGameType() const
{
    return m_gameType;
}

bool SetGameModeS2CPacket::IsFlying() const
{
    return m_flying;
}
