#pragma once

#include "Block.h"

class SlabBlock : public Block
{
public:
    SlabBlock(const std::string &texturePath, Block *fullBlock);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    Block *GetCombinedBlock(uint16_t existingData, uint16_t placedData) const override;

    bool IsTopHalf(uint16_t data) const;

private:
    Block *m_fullBlock;
    int m_halfProperty;
};
