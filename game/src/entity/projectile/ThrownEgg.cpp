#include "ThrownEgg.h"

#include <memory>
#include <utility>

#include "../../world/Level.h"
#include "../../world/item/Item.h"
#include "../../world/particle/ParticleSpawnParams.h"
#include "../animal/Chicken.h"

ThrownEgg::ThrownEgg(Level *level) : ThrowableProjectile(level)
{}

eENTITY_TYPE ThrownEgg::GetType()
{
    return s_type;
}

Item *ThrownEgg::GetItem() const
{
    return Item::ByName("egg");
}

void ThrownEgg::OnHit(Entity *entity)
{
    Random &random = m_level->GetRandom();

    LivingEntity *target = dynamic_cast<LivingEntity *>(entity);
    LivingEntity *owner  = dynamic_cast<LivingEntity *>(m_level->GetEntityById(m_ownerId));
    if (target && target->Hurt(eDamageSource_Thrown, 0, owner) && owner)
    {
        target->Knockback(owner->GetPosition(), glm::dvec3());
    }

    if (random.NextInt(0, 7) == 0)
    {
        int count = random.NextInt(0, 31) == 0 ? 4 : 1;
        for (int i = 0; i < count; i++)
        {
            std::unique_ptr<Chicken> chicken = std::make_unique<Chicken>(m_level);
            chicken->SetAge(-24000);
            chicken->SetPosition(m_position);
            chicken->SetRotation(m_yaw, 0.0f);
            m_level->AddEntity(std::move(chicken));
        }
    }

    m_level->RemoveEntity(this);
}

void ThrownEgg::OnRemoved()
{
    Random &random = m_level->GetRandom();

    const Icon &icon = GetItem()->GetIcon();
    for (int i = 0; i < 8; i++)
    {
        ParticleSpawnParams params;
        params.type        = eParticleType_ItemCrack;
        params.position    = m_position;
        params.velocity    = glm::dvec3((random.NextFloat() - 0.5) * 0.1, random.NextFloat() * 0.1 + 0.05, (random.NextFloat() - 0.5) * 0.1);
        params.hasVelocity = true;
        params.region      = {icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()};
        params.hasRegion   = true;
        m_level->SpawnParticle(params);
    }
}
