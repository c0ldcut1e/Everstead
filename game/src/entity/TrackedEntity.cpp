#include "TrackedEntity.h"

#include <cmath>
#include <cstdlib>
#include <memory>

#include "EntityTracker.h"

#include "player/ServerPlayer.h"

#include "../network/PlayerList.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/Packet.h"
#include "../network/packet/s2c/MoveEntityPosRotS2CPacket.h"
#include "../network/packet/s2c/MoveEntityPosS2CPacket.h"
#include "../network/packet/s2c/MoveEntityRotS2CPacket.h"
#include "../network/packet/s2c/RotateHeadS2CPacket.h"
#include "../network/packet/s2c/SetEntityMotionS2CPacket.h"
#include "../network/packet/s2c/TeleportEntityS2CPacket.h"

#include <glm/geometric.hpp>

const int TrackedEntity::s_resyncInterval         = 400;
const int TrackedEntity::s_rotationToleranceLevel = 1;
const int TrackedEntity::s_headToleranceLevel     = 1;
const int TrackedEntity::s_positionScale          = 4096;
const int TrackedEntity::s_rotationScale          = 256;
const int TrackedEntity::s_minByteValue           = -128;
const int TrackedEntity::s_maxByteValue           = 127;
const float TrackedEntity::s_fullRotation         = 360.0f;

TrackedEntity::TrackedEntity(Entity *entity)
{
    m_entity     = entity;
    m_controller = nullptr;
    m_tickCount  = 0;
    m_xp         = PositionToPacked(m_entity->GetPosition().x);
    m_yp         = PositionToPacked(m_entity->GetPosition().y);
    m_zp         = PositionToPacked(m_entity->GetPosition().z);
    m_yawp       = RotationToPacked(m_entity->GetYaw());
    m_pitchp     = RotationToPacked(m_entity->GetPitch());
    m_headYawp   = RotationToPacked(m_entity->GetHeadYaw());
    m_onGround   = m_entity->IsOnGround();
    m_velocity   = m_entity->GetVelocity();
}

TrackedEntity::~TrackedEntity()
{}

void TrackedEntity::Tick(EntityTracker *tracker)
{
    int tick = m_tickCount++;
    if (tick % m_entity->GetUpdateInterval() != 0)
    {
        return;
    }

    int64_t xn   = PositionToPacked(m_entity->GetPosition().x);
    int64_t yn   = PositionToPacked(m_entity->GetPosition().y);
    int64_t zn   = PositionToPacked(m_entity->GetPosition().z);
    int yawp     = RotationToPacked(m_entity->GetYaw());
    int pitchp   = RotationToPacked(m_entity->GetPitch());
    int headYawp = RotationToPacked(m_entity->GetHeadYaw());

    int64_t xa = xn - m_xp;
    int64_t ya = yn - m_yp;
    int64_t za = zn - m_zp;

    bool onGround = m_entity->ConsumeGroundContact();
    bool moved    = xa != 0 || ya != 0 || za != 0 || onGround != m_onGround;

    int yawa     = WrapPackedRotationDelta(yawp - m_yawp);
    int pitcha   = WrapPackedRotationDelta(pitchp - m_pitchp);
    bool rotated = abs(yawa) >= s_rotationToleranceLevel || abs(pitcha) >= s_rotationToleranceLevel;
    int headYawa = WrapPackedRotationDelta(headYawp - m_headYawp);
    if (abs(headYawa) >= s_headToleranceLevel)
    {
        m_headYawp = headYawp;
        Send(tracker, std::shared_ptr<Packet>(new RotateHeadS2CPacket(m_entity->GetId(), (uint8_t) m_headYawp)));
    }

    if (m_entity->SendsVelocity())
    {
        glm::dvec3 velocity = m_entity->GetVelocity();
        glm::dvec3 change   = velocity - m_velocity;
        double changeSqr    = glm::dot(change, change);
        if (changeSqr > 1.0e-7 || (changeSqr > 0.0 && velocity == glm::dvec3()))
        {
            m_velocity = velocity;
            Send(tracker, std::shared_ptr<Packet>(new SetEntityMotionS2CPacket(m_entity->GetId(), m_velocity)));
        }
    }

    const std::vector<Entity *> &passengers = m_entity->GetPassengers();
    Entity *controller                      = passengers.empty() || !m_entity->IsControlledByRider() ? nullptr : passengers[0];
    bool resync                             = tick % s_resyncInterval < m_entity->GetUpdateInterval() || controller != m_controller;
    m_controller                            = controller;
    if (!moved && !rotated && !resync)
    {
        return;
    }

    if (resync || xa < INT16_MIN || xa > INT16_MAX || ya < INT16_MIN || ya > INT16_MAX || za < INT16_MIN || za > INT16_MAX)
    {
        m_xp       = xn;
        m_yp       = yn;
        m_zp       = zn;
        m_yawp     = yawp;
        m_pitchp   = pitchp;
        m_onGround = onGround;
        Send(tracker, std::shared_ptr<Packet>(
                              new TeleportEntityS2CPacket(m_entity->GetId(), m_xp, m_yp, m_zp, (uint8_t) m_yawp, (uint8_t) m_pitchp, onGround)));
        return;
    }

    if (moved && rotated)
    {
        m_xp       = xn;
        m_yp       = yn;
        m_zp       = zn;
        m_yawp     = yawp;
        m_pitchp   = pitchp;
        m_onGround = onGround;
        Send(tracker, std::shared_ptr<Packet>(new MoveEntityPosRotS2CPacket(m_entity->GetId(), (int16_t) xa, (int16_t) ya, (int16_t) za,
                                                                            (uint8_t) yawa, (uint8_t) pitcha, onGround)));
        return;
    }

    if (moved)
    {
        m_xp       = xn;
        m_yp       = yn;
        m_zp       = zn;
        m_onGround = onGround;
        Send(tracker, std::shared_ptr<Packet>(new MoveEntityPosS2CPacket(m_entity->GetId(), (int16_t) xa, (int16_t) ya, (int16_t) za, onGround)));
        return;
    }

    m_yawp   = yawp;
    m_pitchp = pitchp;
    Send(tracker, std::shared_ptr<Packet>(new MoveEntityRotS2CPacket(m_entity->GetId(), (uint8_t) yawa, (uint8_t) pitcha)));
}

Entity *TrackedEntity::GetEntity() const
{
    return m_entity;
}

void TrackedEntity::Send(EntityTracker *tracker, std::shared_ptr<Packet> packet) const
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = tracker->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i].get() != m_entity && players[i].get() != m_controller && players[i]->GetLevel() == m_entity->GetLevel())
        {
            players[i]->GetPlayerConnection()->Send(packet);
        }
    }
}

int64_t TrackedEntity::PositionToPacked(double value) const
{
    return (int64_t) floor(value * s_positionScale);
}

int TrackedEntity::RotationToPacked(float value) const
{
    return (int) floor(value * s_rotationScale / s_fullRotation);
}

int TrackedEntity::WrapPackedRotationDelta(int value) const
{
    while (value > s_maxByteValue)
    {
        value -= s_rotationScale;
    }

    while (value < s_minByteValue)
    {
        value += s_rotationScale;
    }

    return value;
}
