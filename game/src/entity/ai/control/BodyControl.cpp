#include "BodyControl.h"

#include <algorithm>
#include <cmath>

#include "../../../utils/math/Mth.h"
#include "../../Mob.h"
#include "MoveControl.h"

BodyControl::BodyControl(Mob *mob)
{
    m_mob         = mob;
    m_timeStill   = 0;
    m_lastHeadYaw = 0.0f;
}

void BodyControl::Tick()
{
    const float maxClampAngle = 75.0f;

    double deltaX = m_mob->GetPosition().x - m_mob->GetOldPosition().x;
    double deltaZ = m_mob->GetPosition().z - m_mob->GetOldPosition().z;
    if (deltaX * deltaX + deltaZ * deltaZ > MoveControl::kMinSpeedSquared)
    {
        m_mob->SetBodyYaw(m_mob->GetYaw());
        m_mob->SetHeadYaw(Clamp(m_mob->GetBodyYaw(), m_mob->GetHeadYaw(), maxClampAngle));
        m_lastHeadYaw = m_mob->GetHeadYaw();
        m_timeStill   = 0;
        return;
    }

    float clampAngle = maxClampAngle;
    if (fabsf(m_mob->GetHeadYaw() - m_lastHeadYaw) > 15.0f)
    {
        m_timeStill   = 0;
        m_lastHeadYaw = m_mob->GetHeadYaw();
    }
    else
    {
        m_timeStill++;
        if (m_timeStill > 10)
        {
            clampAngle = std::max(1.0f - (float) (m_timeStill - 10) / 10.0f, 0.0f) * maxClampAngle;
        }
    }

    m_mob->SetBodyYaw(Clamp(m_mob->GetHeadYaw(), m_mob->GetBodyYaw(), clampAngle));
}

float BodyControl::Clamp(float clampTo, float clampFrom, float clampAngle)
{
    float diff = Mth::WrapDegrees(clampTo - clampFrom);
    if (diff < -clampAngle)
    {
        diff = -clampAngle;
    }

    if (diff >= clampAngle)
    {
        diff = clampAngle;
    }

    return clampTo - diff;
}
