#pragma once

#include "Block.h"

class FenceBlock : public Block
{
public:
    FenceBlock(const std::string &texturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void UpdateShape(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;

protected:
    static Direction *const s_sides[4];

    virtual bool ConnectsTo(const Block *block) const;

    int m_sideProperties[4];

private:
    static constexpr double kPostMin = 0.375;
    static constexpr double kPostMax = 0.625;
    static constexpr double kBarMin  = 0.4375;
    static constexpr double kBarMax  = 0.5625;

    uint16_t GetConnections(Level *level, const BlockPos &pos) const;
};
