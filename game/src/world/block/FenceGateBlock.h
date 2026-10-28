#pragma once

#include "OpenableBlock.h"

class FenceGateBlock : public OpenableBlock
{
public:
    FenceGateBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;

private:
    AABB Rotate(uint16_t data, const AABB &box) const;

    int m_facingProperty;
};
