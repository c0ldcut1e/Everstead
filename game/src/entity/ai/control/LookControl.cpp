#include "LookControl.h"

#include <cmath>

#include "../../../utils/math/Mth.h"
#include "../../Mob.h"
#include "../pathfinder/PathNavigation.h"
#include "MoveControl.h"

#include <glm/trigonometric.hpp>

LookControl::LookControl(Mob *mob)
{
    m_mob       = mob;
    m_yawMax    = 0.0f;
    m_pitchMax  = 0.0f;
    m_hasWanted = false;
    m_wantedX   = 0.0;
    m_wantedY   = 0.0;
    m_wantedZ   = 0.0;
}

void LookControl::SetLookAt(Entity *target, float yawMax, float pitchMax)
{
    const AABB &box      = target->GetAABB();
    double y             = (box.GetMin().y + box.GetMax().y) / 2.0;
    LivingEntity *living = dynamic_cast<LivingEntity *>(target);
    if (living)
    {
        y = target->GetPosition().y + living->GetEyeHeight();
    }

    SetLookAt(target->GetPosition().x, y, target->GetPosition().z, yawMax, pitchMax);
}

void LookControl::SetLookAt(double x, double y, double z, float yawMax, float pitchMax)
{
    m_wantedX   = x;
    m_wantedY   = y;
    m_wantedZ   = z;
    m_yawMax    = yawMax;
    m_pitchMax  = pitchMax;
    m_hasWanted = true;
}

void LookControl::Tick()
{
    float pitch   = 0.0f;
    float headYaw = m_mob->GetHeadYaw();
    float bodyYaw = m_mob->GetBodyYaw();
    if (m_hasWanted)
    {
        m_hasWanted = false;

        const glm::dvec3 &position = m_mob->GetPosition();
        double deltaX              = m_wantedX - position.x;
        double deltaY              = m_wantedY - (position.y + m_mob->GetEyeHeight());
        double deltaZ              = m_wantedZ - position.z;
        double horizontal          = sqrt(deltaX * deltaX + deltaZ * deltaZ);
        pitch                      = MoveControl::RotLerp(0.0f, glm::degrees((float) atan2(deltaY, horizontal)), m_pitchMax);
        headYaw                    = MoveControl::RotLerp(headYaw, glm::degrees((float) atan2(deltaZ, deltaX)), m_yawMax);
    }
    else
    {
        headYaw = MoveControl::RotLerp(headYaw, bodyYaw, 10.0f);
    }

    float headToBody = Mth::WrapDegrees(headYaw - bodyYaw);
    if (!m_mob->GetNavigation()->IsDone())
    {
        if (headToBody < -75.0f)
        {
            headYaw = bodyYaw - 75.0f;
        }

        if (headToBody > 75.0f)
        {
            headYaw = bodyYaw + 75.0f;
        }
    }

    m_mob->SetRotation(m_mob->GetYaw(), pitch);
    m_mob->SetHeadYaw(headYaw);
}
