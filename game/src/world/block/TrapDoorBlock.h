#pragma once

#include "OpenableBlock.h"

class TrapDoorBlock : public OpenableBlock
{
public:
    TrapDoorBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;

private:
    int m_halfProperty;
};
