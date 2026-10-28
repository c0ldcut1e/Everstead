#include "Snowball.h"

#include "../../world/Level.h"
#include "../../world/item/Item.h"
#include "../../world/particle/ParticleSpawnParams.h"
#include "../player/ServerPlayer.h"

Snowball::Snowball(Level *level) : ThrowableProjectile(level)
{}

eENTITY_TYPE Snowball::GetType()
{
    return s_type;
}

Item *Snowball::GetItem() const
{
    return Item::ByName("snowball");
}

void Snowball::OnHit(Entity *entity)
{
    ServerPlayer *target = dynamic_cast<ServerPlayer *>(entity);
    ServerPlayer *owner  = dynamic_cast<ServerPlayer *>(m_level->GetEntityById(m_ownerId));
    if (target && target->Hurt(eDamageSource_Thrown, 0, owner) && owner)
    {
        target->Knockback(owner->GetPosition(), glm::dvec3());
    }

    m_level->RemoveEntity(this);
}

void Snowball::OnRemoved()
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
