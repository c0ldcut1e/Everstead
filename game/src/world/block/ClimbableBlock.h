#pragma once

#include "Block.h"

class ClimbableBlock : public Block
{
public:
    ClimbableBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    Block *GetPlacementBlock(Direction *face) override;

private:
    static constexpr float kThickness = 0.05f;
};
