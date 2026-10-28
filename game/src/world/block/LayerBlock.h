#pragma once

#include "Block.h"

class LayerBlock : public Block
{
public:
    LayerBlock(const std::string &texturePath, Block *fullBlock);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    bool CanBeReplaced(uint16_t data) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    Block *GetCombinedBlock(uint16_t existingData, uint16_t placedData) const override;

    int GetLayers(uint16_t data) const;
    uint16_t WithLayers(int layers) const;

    static constexpr int kMaxLayers = 8;

private:
    Block *m_fullBlock;
    int m_layersProperty;
};
