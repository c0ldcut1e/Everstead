#include "MoveControl.h"

#include <algorithm>
#include <cmath>

#include "../../../utils/math/Mth.h"
#include "../../Mob.h"
#include "JumpControl.h"

#include <glm/trigonometric.hpp>

MoveControl::MoveControl(Mob *mob)
{
    m_mob       = mob;
    m_wantedX   = mob->GetPosition().x;
    m_wantedY   = mob->GetPosition().y;
    m_wantedZ   = mob->GetPosition().z;
    m_speed     = 0.0;
    m_hasWanted = false;
}

bool MoveControl::HasWanted() const
{
    return m_hasWanted;
}

double MoveControl::GetSpeed() const
{
    return m_speed;
}

glm::dvec3 MoveControl::GetWantedPosition() const
{
    return glm::dvec3(m_wantedX, m_wantedY, m_wantedZ);
}

void MoveControl::SetWantedPosition(double x, double y, double z, double speed)
{
    m_wantedX   = x;
    m_wantedY   = y;
    m_wantedZ   = z;
    m_speed     = speed;
    m_hasWanted = true;
}

void MoveControl::Tick()
{
    m_mob->SetForwardInput(0.0);
    if (!m_hasWanted)
    {
        return;
    }

    m_hasWanted = false;

    const glm::dvec3 &position = m_mob->GetPosition();
    double deltaX              = m_wantedX - position.x;
    double deltaY              = m_wantedY - floor(m_mob->GetAABB().GetMin().y + 0.5);
    double deltaZ              = m_wantedZ - position.z;
    if (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ < kMinSpeedSquared)
    {
        return;
    }

    m_mob->SetRotation(RotLerp(m_mob->GetYaw(), glm::degrees((float) atan2(deltaZ, deltaX)), 30.0f), m_mob->GetPitch());
    m_mob->SetSpeed(m_speed);
    if (deltaY > 0.0 && deltaX * deltaX + deltaZ * deltaZ < 1.0)
    {
        m_mob->GetJumpControl()->Jump();
    }
}

float MoveControl::RotLerp(float from, float to, float max)
{
    return from + std::clamp(Mth::WrapDegrees(to - from), -max, max);
}
