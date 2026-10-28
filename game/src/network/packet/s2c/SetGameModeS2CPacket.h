#pragma once

#include "../../../entity/player/GameType.h"
#include "../Packet.h"

class SetGameModeS2CPacket : public Packet
{
public:
    SetGameModeS2CPacket();
    explicit SetGameModeS2CPacket(eGAME_TYPE gameType, bool flying);
    ~SetGameModeS2CPacket() override;

    ePACKET_ID GetId() override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) override;
    void Handle(PacketListener *listener) override;
    int GetEstimatedSize() override;

    eGAME_TYPE GetGameType() const;
    bool IsFlying() const;

private:
    eGAME_TYPE m_gameType;
    bool m_flying;
};
