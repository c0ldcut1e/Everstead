#include "Creeper.h"

#include <algorithm>

#include "../../world/Explosion.h"
#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/HurtByTargetGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/MeleeAttackGoal.h"
#include "../ai/goal/NearestAttackableTargetGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../ai/goal/SwellGoal.h"

Creeper::Creeper(Level *level) : Monster(level)
{
    m_oldSwell = 0;
    m_swell    = 0;
    m_swellDir = -1;
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.25);
    m_goalSelector.AddGoal(1, new FloatGoal(this));
    m_goalSelector.AddGoal(2, new SwellGoal(this));
    m_goalSelector.AddGoal(4, new MeleeAttackGoal(this, 1.0, false));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 0.8));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 8.0f));
    m_goalSelector.AddGoal(6, new RandomLookAroundGoal(this));
    m_targetSelector.AddGoal(1, new NearestAttackableTargetGoal(this, 16.0, true));
    m_targetSelector.AddGoal(2, new HurtByTargetGoal(this));
}

eENTITY_TYPE Creeper::GetType()
{
    return s_type;
}

bool Creeper::DoHurtTarget(LivingEntity *target)
{
    (void) target;
    return true;
}

void Creeper::GetMobData(MobData *data) const
{
    Monster::GetMobData(data);
    data->variant = m_swellDir > 0 ? 1 : 0;
}

void Creeper::ApplyMobData(const MobData &data)
{
    Monster::ApplyMobData(data);
    m_swellDir = data.variant ? 1 : -1;
}

int Creeper::GetSwellDir() const
{
    return m_swellDir;
}

void Creeper::SetSwellDir(int swellDir)
{
    m_swellDir = swellDir;
}

float Creeper::GetSwelling(float partialTick) const
{
    return (m_oldSwell + (m_swell - m_oldSwell) * partialTick) / 28.0f;
}

void Creeper::ServerTick()
{
    if (m_health > 0)
    {
        if (m_swellDir > 0 && m_swell == 0)
        {
            m_level->PlaySound("random.fuse", m_position, 1.0f, 0.5f);
        }

        TickSwell();
        if (m_swell >= 30)
        {
            Explosion explosion(m_level, m_position, 3.0f);
            explosion.Explode();
            m_level->RemoveEntity(this);
            return;
        }
    }

    Monster::ServerTick();
}

void Creeper::ClientTick()
{
    TickSwell();
    Monster::ClientTick();
}

void Creeper::TickSwell()
{
    m_oldSwell = m_swell;
    m_swell    = std::clamp(m_swell + m_swellDir, 0, 30);
}

std::string Creeper::GetHurtSound() const
{
    return "mob.creeper.say" + std::to_string(GetRandom().NextInt(1, 4));
}

std::string Creeper::GetDeathSound() const
{
    return "mob.creeper.death";
}
