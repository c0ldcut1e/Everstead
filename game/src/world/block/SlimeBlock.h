#pragma once

#include "Block.h"

class SlimeBlock : public Block
{
public:
    SlimeBlock();

    void OnEntityLanded(Entity *entity, double velocityY) const override;
    void OnEntityWalk(Entity *entity) const override;
    float GetFallDamageMultiplier(Entity *entity) const override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
};
