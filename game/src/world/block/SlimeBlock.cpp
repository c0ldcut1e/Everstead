#include "SlimeBlock.h"

#include <cmath>

#include "../../entity/LivingEntity.h"

SlimeBlock::SlimeBlock() : Block(true, "assets/texture/block/slime_block.png")
{
    SetTransparent(true);
    SetTranslucent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetSlipperiness(0.8f);
    SetDestroyTime(0.0f);
    SetBreakSound("mob.slime.big", 4);
    SetPlaceSound("mob.slime.big", 4, 1.0f);
}

void SlimeBlock::OnEntityLanded(Entity *entity, double velocityY) const
{
    if (entity->IsSneaking() || velocityY >= 0.0)
    {
        return;
    }

    glm::dvec3 velocity = entity->GetVelocity();
    velocity.y          = dynamic_cast<LivingEntity *>(entity) ? -velocityY : -velocityY * 0.8;
    entity->SetVelocity(velocity);
}

void SlimeBlock::OnEntityWalk(Entity *entity) const
{
    glm::dvec3 velocity = entity->GetVelocity();
    if (std::abs(velocity.y) < 0.1)
    {
        double factor = 0.4 + std::abs(velocity.y) * 0.2;
        entity->SetVelocity(glm::dvec3(velocity.x * factor, velocity.y, velocity.z * factor));
    }
}

float SlimeBlock::GetFallDamageMultiplier(Entity *entity) const
{
    return entity->IsSneaking() ? 1.0f : 0.0f;
}

void SlimeBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0)));
}

void SlimeBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(3.0 / 16.0), glm::dvec3(13.0 / 16.0)));
    out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0)));
}
