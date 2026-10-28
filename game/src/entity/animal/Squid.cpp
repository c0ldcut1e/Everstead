#include "Squid.h"

#include <cmath>
#include <numbers>

#include <glm/trigonometric.hpp>

Squid::Squid(Level *level) : Mob(level)
{
    SetSize(0.95f, 0.95f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(10.0);
    SetHealth(10);
    m_xBodyRot         = 0.0f;
    m_xBodyRotOld      = 0.0f;
    m_zBodyRot         = 0.0f;
    m_zBodyRotOld      = 0.0f;
    m_tentacleMovement = 0.0f;
    m_tentacleAngle    = 0.0f;
    m_tentacleAngleOld = 0.0f;
    m_speed            = 0.0f;
    m_tentacleSpeed    = 1.0f / (GetRandom().NextFloat() + 1.0f) * 0.2f;
    m_rotateSpeed      = 0.0f;
    m_swimTarget       = glm::dvec3();
}

eENTITY_TYPE Squid::GetType()
{
    return s_type;
}

bool Squid::CanBeLeashed() const
{
    return false;
}

float Squid::GetXBodyRot(float partialTicks) const
{
    return m_xBodyRotOld + (m_xBodyRot - m_xBodyRotOld) * partialTicks;
}

float Squid::GetZBodyRot(float partialTicks) const
{
    return m_zBodyRotOld + (m_zBodyRot - m_zBodyRotOld) * partialTicks;
}

float Squid::GetTentacleAngle(float partialTicks) const
{
    return m_tentacleAngleOld + (m_tentacleAngle - m_tentacleAngleOld) * partialTicks;
}

void Squid::ServerTick()
{
    if (m_noActionTime > 100)
    {
        m_swimTarget = glm::dvec3();
    }
    else if (GetRandom().NextInt(0, 49) == 0 || !IsInWater() || m_swimTarget == glm::dvec3())
    {
        float angle  = GetRandom().NextFloat() * (float) std::numbers::pi * 2.0f;
        m_swimTarget = glm::dvec3(cosf(angle) * 0.2f, -0.1f + GetRandom().NextFloat() * 0.2f, sinf(angle) * 0.2f);
    }

    TickSwim(m_velocity);
    if (IsInWater())
    {
        m_velocity     = m_swimTarget * (double) m_speed;
        float yawDelta = glm::degrees(atan2f((float) m_swimTarget.z, (float) m_swimTarget.x)) - m_yaw;
        while (yawDelta < -180.0f)
        {
            yawDelta += 360.0f;
        }

        while (yawDelta >= 180.0f)
        {
            yawDelta -= 360.0f;
        }

        m_yaw += yawDelta * 0.1f;
        m_headYaw = m_yaw;
        m_bodyYaw = m_yaw;
    }
    else
    {
        m_velocity.x = 0.0;
        m_velocity.z = 0.0;
        m_velocity.y = (m_velocity.y - 0.08) * 0.98;
    }

    Mob::ServerTick();
}

void Squid::ClientTick()
{
    Mob::ClientTick();
    TickSwim(m_position - m_oldPosition);
}

void Squid::TickSwim(const glm::dvec3 &motion)
{
    m_xBodyRotOld      = m_xBodyRot;
    m_zBodyRotOld      = m_zBodyRot;
    m_tentacleAngleOld = m_tentacleAngle;
    m_tentacleMovement += m_tentacleSpeed;
    if (m_tentacleMovement > (float) std::numbers::pi * 2.0f)
    {
        m_tentacleMovement -= (float) std::numbers::pi * 2.0f;
        if (GetRandom().NextInt(0, 9) == 0)
        {
            m_tentacleSpeed = 1.0f / (GetRandom().NextFloat() + 1.0f) * 0.2f;
        }
    }

    if (!IsInWater())
    {
        m_tentacleAngle = fabsf(sinf(m_tentacleMovement)) * (float) std::numbers::pi * 0.25f;
        m_xBodyRot += (-90.0f - m_xBodyRot) * 0.02f;
        return;
    }

    if (m_tentacleMovement < (float) std::numbers::pi)
    {
        float progress  = m_tentacleMovement / (float) std::numbers::pi;
        m_tentacleAngle = sinf(progress * progress * (float) std::numbers::pi) * (float) std::numbers::pi * 0.25f;
        if (progress > 0.75f)
        {
            m_speed       = 1.0f;
            m_rotateSpeed = 1.0f;
        }
        else
        {
            m_rotateSpeed *= 0.8f;
        }
    }
    else
    {
        m_tentacleAngle = 0.0f;
        m_speed *= 0.9f;
        m_rotateSpeed *= 0.99f;
    }

    float horizontal = (float) sqrt(motion.x * motion.x + motion.z * motion.z);
    m_zBodyRot += (float) std::numbers::pi * m_rotateSpeed * 1.5f;
    m_xBodyRot += (-glm::degrees(atan2f(horizontal, (float) motion.y)) - m_xBodyRot) * 0.1f;
}

void Squid::Travel()
{
    Move(m_velocity.x, m_velocity.y, m_velocity.z);
}

bool Squid::TickAirSupply()
{
    if (m_health <= 0 || IsInWater())
    {
        m_airSupply = s_totalAirSupply;
        return false;
    }

    m_airSupply--;
    if (m_airSupply == -20)
    {
        m_airSupply = 0;
        return true;
    }

    return false;
}

bool Squid::RemoveWhenFarAway() const
{
    return true;
}

int Squid::GetExperienceReward()
{
    return 1 + GetRandom().NextInt(0, 2);
}

int Squid::GetAmbientSoundInterval() const
{
    return 120;
}

float Squid::GetSoundVolume() const
{
    return 0.4f;
}

std::string Squid::GetHurtSound() const
{
    return "";
}

std::string Squid::GetDeathSound() const
{
    return "";
}
