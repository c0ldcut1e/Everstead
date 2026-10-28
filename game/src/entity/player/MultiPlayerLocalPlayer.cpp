#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MultiPlayerLocalPlayer.h"

#include <cmath>
#include <memory>

#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/MovePlayerC2SPacket.h"
#include "../../network/packet/c2s/MoveVehicleC2SPacket.h"
#include "../../network/packet/c2s/PlayerActionC2SPacket.h"
#include "../../network/packet/c2s/PlayerInputC2SPacket.h"
#include "../../network/packet/c2s/SetCarriedItemC2SPacket.h"
#include "../../network/packet/c2s/SwingC2SPacket.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRegistry.h"
#include "../item/Boat.h"
#include "../item/Minecart.h"

const double MultiPlayerLocalPlayer::s_movePositionThreshold = 0.05;
const float MultiPlayerLocalPlayer::s_moveRotationThreshold  = 0.5f;

MultiPlayerLocalPlayer::MultiPlayerLocalPlayer(Level *level, const std::wstring &name) : LocalPlayer(level, name)
{
    m_clientConnection     = nullptr;
    m_lastSentPosition     = glm::dvec3();
    m_lastSentYaw          = 0.0f;
    m_lastSentPitch        = 0.0f;
    m_lastSentOnGround     = false;
    m_lastSentFlying       = false;
    m_ticksSinceMoveSent   = s_moveKeepAliveTicks;
    m_lastSentSprinting    = false;
    m_lastSentSneaking     = false;
    m_lastSentCarriedSlot  = -1;
    m_lastSentForwardInput = 0.0f;
}

MultiPlayerLocalPlayer::~MultiPlayerLocalPlayer()
{
    delete m_clientConnection;
}

eENTITY_TYPE MultiPlayerLocalPlayer::GetType()
{
    return s_type;
}

void MultiPlayerLocalPlayer::Tick()
{
    if (m_clientConnection)
    {
        m_clientConnection->Tick();
    }

    // Unloaded chunks collide as solid, so hold still until the chunk under a fresh teleport arrives
    ChunkPos chunkPos((int) floor(m_position.x) >> 4, (int) floor(m_position.z) >> 4);
    if (!m_level->HasChunk(chunkPos))
    {
        SetVelocity(glm::dvec3());
        StoreOld();
        SendMoveIfNeeded();
        return;
    }

    LocalPlayer::Tick();
    SendMoveIfNeeded();
    Boat *boat = dynamic_cast<Boat *>(m_vehicle);
    if (m_clientConnection && boat && boat->GetPassengers()[0] == this)
    {
        glm::dvec3 position = boat->GetPosition();
        m_clientConnection->Send(std::shared_ptr<Packet>(
                new MoveVehicleC2SPacket(position.x, position.y, position.z, boat->GetYaw(), boat->GetPaddleState(0), boat->GetPaddleState(1))));
    }

    if (m_clientConnection && dynamic_cast<Minecart *>(m_vehicle) && GetMoveForwardInput() != m_lastSentForwardInput)
    {
        m_lastSentForwardInput = GetMoveForwardInput();
        m_clientConnection->Send(std::shared_ptr<Packet>(new PlayerInputC2SPacket(m_lastSentForwardInput)));
    }

    SendActionFlagsIfNeeded();
    SendCarriedItemIfNeeded();
}

void MultiPlayerLocalPlayer::Swing()
{
    bool sendSwing = CanStartSwing();
    LocalPlayer::Swing();
    if (m_clientConnection && sendSwing)
    {
        m_clientConnection->Send(std::shared_ptr<Packet>(new SwingC2SPacket()));
    }
}

void MultiPlayerLocalPlayer::UseBlock(const BlockPos &pos, Direction *face)
{
    if (!m_clientConnection)
    {
        LocalPlayer::UseBlock(pos, face);
        return;
    }

    SendCarriedItemIfNeeded();
    m_clientConnection->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_Use, pos, face->GetId(), 0)));
}

void MultiPlayerLocalPlayer::UseItemOn(const BlockPos &pos, Direction *face)
{
    if (!m_clientConnection)
    {
        LocalPlayer::UseItemOn(pos, face);
        return;
    }

    SendCarriedItemIfNeeded();
    m_clientConnection->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_UseItem, pos, face->GetId(), 0)));
}

void MultiPlayerLocalPlayer::SetBlockByPlayer(const BlockPos &pos, Block *block, Direction *placedAgainst, uint16_t data)
{
    if (!m_clientConnection)
    {
        LocalPlayer::SetBlockByPlayer(pos, block, placedAgainst, data);
        return;
    }

    LocalPlayer::SetBlockByPlayer(pos, block, placedAgainst, data);

    SendCarriedItemIfNeeded();
    uint8_t placedAgainstFace = placedAgainst ? placedAgainst->GetId() : 0;
    if (!block || block == Block::ByName("air"))
    {
        m_clientConnection->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_Break, pos, 0, 0)));
    }
    else
    {
        m_clientConnection->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_Place, pos, placedAgainstFace, data)));
    }
}

void MultiPlayerLocalPlayer::SendMoveIfNeeded()
{
    if (!m_clientConnection)
    {
        return;
    }

    double dx         = m_position.x - m_lastSentPosition.x;
    double dy         = m_position.y - m_lastSentPosition.y;
    double dz         = m_position.z - m_lastSentPosition.z;
    double distanceSq = dx * dx + dy * dy + dz * dz;
    bool moved        = distanceSq > s_movePositionThreshold * s_movePositionThreshold;
    float yawDelta    = fabsf(m_yaw - m_lastSentYaw);
    float pitchDelta  = fabsf(m_pitch - m_lastSentPitch);
    bool rotated      = yawDelta > s_moveRotationThreshold || pitchDelta > s_moveRotationThreshold;
    m_ticksSinceMoveSent++;
    if (!moved && !rotated && m_onGround == m_lastSentOnGround && m_flying == m_lastSentFlying && m_ticksSinceMoveSent < s_moveKeepAliveTicks)
    {
        return;
    }

    m_clientConnection->Send(std::shared_ptr<Packet>(new MovePlayerC2SPacket(m_position.x, m_position.y, m_position.z, m_yaw, m_pitch, m_onGround,
                                                                             m_flying, m_clientConnection->GetTeleportId())));
    m_lastSentPosition   = m_position;
    m_lastSentYaw        = m_yaw;
    m_lastSentPitch      = m_pitch;
    m_lastSentOnGround   = m_onGround;
    m_lastSentFlying     = m_flying;
    m_ticksSinceMoveSent = 0;
}

void MultiPlayerLocalPlayer::SendActionFlagsIfNeeded()
{
    if (!m_clientConnection)
    {
        return;
    }

    if (IsSprinting() != m_lastSentSprinting)
    {
        m_clientConnection->Send(std::shared_ptr<Packet>(
                new PlayerActionC2SPacket(IsSprinting() ? ePlayerAction_StartSprinting : ePlayerAction_StopSprinting, BlockPos(0, 0, 0), 0, 0)));
        m_lastSentSprinting = IsSprinting();
    }

    if (IsSneaking() != m_lastSentSneaking)
    {
        m_clientConnection->Send(std::shared_ptr<Packet>(
                new PlayerActionC2SPacket(IsSneaking() ? ePlayerAction_StartSneaking : ePlayerAction_StopSneaking, BlockPos(0, 0, 0), 0, 0)));
        m_lastSentSneaking = IsSneaking();
    }
}

void MultiPlayerLocalPlayer::SendCarriedItemIfNeeded()
{
    if (!m_clientConnection)
    {
        return;
    }

    int selectedSlot = GetInventory()->GetSelectedSlot();
    if (selectedSlot == m_lastSentCarriedSlot)
    {
        return;
    }

    m_clientConnection->Send(std::shared_ptr<Packet>(new SetCarriedItemC2SPacket((uint8_t) selectedSlot)));
    m_lastSentCarriedSlot = selectedSlot;
}

void MultiPlayerLocalPlayer::SetClientConnection(ClientConnection *clientConnection)
{
    delete m_clientConnection;
    m_clientConnection = clientConnection;
}

ClientConnection *MultiPlayerLocalPlayer::GetClientConnection() const
{
    return m_clientConnection;
}

#endif
