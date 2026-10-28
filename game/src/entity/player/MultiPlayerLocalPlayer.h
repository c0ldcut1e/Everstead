#pragma once

#include "../../network/listener/ClientConnection.h"
#include "LocalPlayer.h"

class MultiPlayerLocalPlayer : public LocalPlayer
{
public:
    MultiPlayerLocalPlayer(Level *level, const std::wstring &name);
    ~MultiPlayerLocalPlayer() override;

    eENTITY_TYPE GetType() override;

    void Tick() override;
    void Swing() override;

    void SetClientConnection(ClientConnection *clientConnection);
    ClientConnection *GetClientConnection() const;
    void SendCarriedItemIfNeeded();

    static const eENTITY_TYPE s_type = eEntityType_MultiPlayerLocalPlayer;

private:
    static const int s_moveKeepAliveTicks = 40;
    static const double s_movePositionThreshold;
    static const float s_moveRotationThreshold;
    void UseBlock(const BlockPos &pos, Direction *face) override;
    void UseItemOn(const BlockPos &pos, Direction *face) override;
    void SetBlockByPlayer(const BlockPos &pos, Block *block, Direction *placedAgainst, uint16_t data) override;
    void SendMoveIfNeeded();
    void SendActionFlagsIfNeeded();

    ClientConnection *m_clientConnection;
    glm::dvec3 m_lastSentPosition;
    float m_lastSentYaw;
    float m_lastSentPitch;
    bool m_lastSentOnGround;
    bool m_lastSentFlying;
    int m_ticksSinceMoveSent;
    bool m_lastSentSprinting;
    bool m_lastSentSneaking;
    int m_lastSentCarriedSlot;
    float m_lastSentForwardInput;
};
