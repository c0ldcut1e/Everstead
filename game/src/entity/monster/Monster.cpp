#include "Monster.h"

Monster::Monster(Level *level) : Mob(level)
{
    m_attributes.Get(Attribute::s_attackDamage)->SetBaseValue(2.0);
}

float Monster::GetWalkTargetValue(const BlockPos &pos) const
{
    return 0.5f - m_level->GetBrightness(pos);
}

bool Monster::CanBeLeashed() const
{
    return false;
}

void Monster::ServerTick()
{
    if (m_level->GetDifficulty() == 0)
    {
        m_level->RemoveEntity(this);
        return;
    }

    if (GetBrightness() > 0.5f)
    {
        m_noActionTime += 2;
    }

    Mob::ServerTick();
}

int Monster::GetExperienceReward()
{
    return 5;
}

bool Monster::RemoveWhenFarAway() const
{
    return true;
}
