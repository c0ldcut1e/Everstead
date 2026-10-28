#pragma once

#include <string>

#include "Block.h"

class StemBlock;

class AttachedStemBlock : public Block
{
public:
    AttachedStemBlock(const std::string &texturePath, const std::string &fruitName, StemBlock *stem);

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const override;

private:
    std::string m_fruitName;
    StemBlock *m_stem;
};
