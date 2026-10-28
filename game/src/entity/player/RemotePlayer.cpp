#ifndef EVERSTEAD_DEDICATED_SERVER

#include "RemotePlayer.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../world/item/Item.h"
#include "../TrackedEntity.h"

RemotePlayer::RemotePlayer(Level *level, const std::wstring &name) : Player(level, name)
{
    m_silent = false;
    m_riding = false;
    SetNoGravity(true);
    SetNoCollision(true);
}

RemotePlayer::~RemotePlayer()
{}

eENTITY_TYPE RemotePlayer::GetType()
{
    return s_type;
}

void RemotePlayer::SetSilent(bool silent)
{
    m_silent = silent;
}

void RemotePlayer::SetRiding(bool riding)
{
    m_riding = riding;
}

bool RemotePlayer::IsRiding() const
{
    return m_riding;
}

void RemotePlayer::ApplyMoveEntity(int xa, int ya, int za, bool hasPosition, int yaw, int pitch, bool hasRotation, bool onGround)
{
    Entity::ApplyMoveEntity(xa, ya, za, hasPosition, yaw, pitch, hasRotation, onGround);
    if (hasPosition && !m_silent && !GetSharedFlag(eEntityFlag_Spectator))
    {
        UpdateFall(m_targetPosition, (double) ya / TrackedEntity::s_positionScale, onGround);
    }
}

void RemotePlayer::ApplyTeleportEntity(int64_t x, int64_t y, int64_t z, int yaw, int pitch, bool onGround)
{
    int64_t ya = y - m_yp;
    Entity::ApplyTeleportEntity(x, y, z, yaw, pitch, onGround);
    if (!m_silent && !GetSharedFlag(eEntityFlag_Spectator))
    {
        UpdateFall(m_targetPosition, (double) ya / TrackedEntity::s_positionScale, onGround);
    }
}

void RemotePlayer::Tick()
{
    Everstead::GetInstance()->GetAccountService()->ApplyPlayerLook(this);

    if (!m_hasNetworkState)
    {
        return;
    }

    m_totalTickCount++;
    TickHurt();
    TickCrit();
    TickDeath();
    if (!m_silent)
    {
        m_riding           = m_vehicle != nullptr;
        ItemInstance *held = m_inventory.GetSelectedItem();
        if (!GetSharedFlag(eEntityFlag_UsingItem))
        {
            StopUsingItem();
        }
        else if (!IsUsingItem() && !held->IsEmpty())
        {
            StartUsingItem(*held, held->GetItem()->GetUseDuration());
        }

        TickUsingItem();
    }

    TickRemote();
    // Rider position packets trail the vehicle, so we place the rider on the vehicle instead
    if (m_vehicle)
    {
        m_vehicle->PositionRider(this);
    }

    float deltaX = 0.0f;
    float deltaZ = 0.0f;
    if (!m_riding)
    {
        deltaX = (float) (GetPosition().x - GetOldPosition().x);
        deltaZ = (float) (GetPosition().z - GetOldPosition().z);
    }

    float horizontalSpeed = sqrtf(deltaX * deltaX + deltaZ * deltaZ);
    UpdateLimbSwing(horizontalSpeed);

    m_walkDist += horizontalSpeed * 0.6f;
    if (!m_silent && !GetSharedFlag(eEntityFlag_Spectator))
    {
        UpdateStepSound(GetPosition() - GetOldPosition(), m_grounded);
        UpdateWaterSplash(GetPosition() - GetOldPosition());
    }
    if (IsSprinting() && m_grounded && horizontalSpeed > 0.01f)
    {
        SpawnSprintParticle(glm::dvec3(deltaX, 0.0, deltaZ));
    }

    m_walkBobOld        = m_walkBob;
    float targetWalkBob = m_grounded ? std::min(0.1f, horizontalSpeed) : 0.0f;
    m_walkBob += (targetWalkBob - m_walkBob) * 0.4f;

    UpdateBodyYaw(deltaX, deltaZ);
    UpdateCapePosition();
    UpdateSwing();
}

float RemotePlayer::GetRenderBob(float partialTicks) const
{
    return (float) m_totalTickCount + partialTicks;
}

#endif
