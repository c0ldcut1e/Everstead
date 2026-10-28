#include "ThrowableProjectile.h"

#include "../../utils/HitResult.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/Block.h"

#include <glm/geometric.hpp>

ThrowableProjectile::ThrowableProjectile(Level *level) : Entity(level)
{
    m_ownerId     = 0;
    m_removeTicks = 0;
    m_noGravity   = true;
    m_boundingBox = {glm::dvec3(-0.125, 0.0, -0.125), glm::dvec3(0.125, 0.25, 0.125)};
}

void ThrowableProjectile::Tick()
{
    m_totalTickCount++;
    if (m_removeTicks > 0)
    {
        m_removeTicks--;
        if (m_removeTicks == 0)
        {
            m_level->RemoveEntity(this);
        }

        return;
    }

    if (m_position.y < -64.0)
    {
        m_level->RemoveEntity(this);
        return;
    }

    double length        = glm::length(m_velocity);
    glm::dvec3 direction = glm::normalize(m_velocity);
    HitResult *hit       = m_level->Clip(m_position, m_velocity, (float) length, false, false, true);
    bool hitBlock        = hit->IsBlock();
    double nearest       = hitBlock ? hit->GetDistance() : length;
    BlockPos hitBlockPos = hit->GetBlockPos();
    Direction *hitFace   = hit->GetBlockFace();
    glm::dvec3 hitPos    = hit->GetHitPos();
    delete hit;

    Entity *hitEntity                                    = nullptr;
    const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (!entity->IsPickable() || (entity->GetId() == m_ownerId && m_totalTickCount < 5))
        {
            continue;
        }

        double distance;
        if (entity->GetAABB().Inflated(0.3).Clip(m_position, direction, nearest, &distance))
        {
            hitEntity = entity;
            nearest   = distance;
        }
    }

    if (hitEntity || hitBlock)
    {
        SetPosition(m_position + direction * nearest);
        m_velocity = glm::dvec3();
        if (!m_level->IsClientSide())
        {
            ((ServerLevel *) m_level)->SendEntity(this, nullptr);
            if (!hitEntity)
            {
                Block::ById(m_level->GetBlockId(hitBlockPos))->OnProjectileHit(m_level, hitBlockPos, this, hitPos, hitFace);
            }

            OnHit(hitEntity);
        }

        return;
    }

    SetPosition(m_position + m_velocity);
    m_velocity *= (IsInWater() ? 0.8 : 0.99);
    m_velocity.y -= 0.03;
}

int ThrowableProjectile::GetUpdateInterval() const
{
    return 10;
}

bool ThrowableProjectile::SendsVelocity() const
{
    return true;
}

bool ThrowableProjectile::IsPickable() const
{
    return false;
}

double ThrowableProjectile::GetRenderDistanceScale() const
{
    return 4.0;
}

void ThrowableProjectile::ReadSpawnData(int data)
{
    m_ownerId = data;
}

int ThrowableProjectile::GetSpawnData()
{
    return m_ownerId;
}

// The removal packet arrives in the same tick as the impact position, so we keep the projectile one more tick to draw its last move
void ThrowableProjectile::RemoveAfterImpact()
{
    m_removeTicks = 2;
}

void ThrowableProjectile::Shoot(Entity *owner)
{
    Random &random = m_level->GetRandom();

    m_ownerId        = owner->GetId();
    glm::dvec3 front = glm::normalize(owner->GetFront());
    glm::dvec3 direction =
            glm::dvec3(front.x + random.NextGaussian() * 0.0075, front.y + random.NextGaussian() * 0.0075, front.z + random.NextGaussian() * 0.0075);
    SetPosition(owner->GetPosition() + glm::dvec3(0.0, 1.52, 0.0));
    SetRotation(owner->GetYaw(), owner->GetPitch());
    SetVelocity(direction * 1.5);
}

int ThrowableProjectile::GetOwnerId() const
{
    return m_ownerId;
}
