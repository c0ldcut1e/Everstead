#pragma once

#include "Block.h"

class ScaffoldingBlock : public Block
{
public:
    ScaffoldingBlock();

    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    bool HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    bool HasAmbientOcclusion() const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    int GetDistance(Level *level, const BlockPos &pos) const;
    bool IsBottom(uint16_t data) const;

private:
    int m_distanceProperty;
    int m_bottomProperty;
};
