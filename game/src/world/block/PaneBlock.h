#pragma once

#include "Block.h"

class PaneBlock : public Block
{
public:
    PaneBlock(const std::string &texturePath, const std::string &edgeTexturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void UpdateShape(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;

private:
    static constexpr double kMin = 0.4375;
    static constexpr double kMax = 0.5625;

    static bool ConnectsTo(const Block *block);

    uint16_t GetConnections(Level *level, const BlockPos &pos) const;

    int m_sideProperties[4];
};
