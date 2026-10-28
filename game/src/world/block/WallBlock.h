#pragma once

#include "FenceBlock.h"

class WallBlock : public FenceBlock
{
public:
    WallBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;

protected:
    bool ConnectsTo(const Block *block) const override;

private:
    static constexpr double kPostMin = 0.25;
    static constexpr double kPostMax = 0.75;
    static constexpr double kArmMin  = 0.3125;
    static constexpr double kArmMax  = 0.6875;

    void AddBoxes(uint16_t data, double postHeight, double armHeight, std::vector<AABB> *out) const;
};
