#pragma once

#include "Block.h"

class PricklyVineBlock : public Block
{
public:
    PricklyVineBlock(const std::string &texturePrefix);

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    Direction *GetAttachment(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    UVRect GetCrossAtlasUVRect(uint16_t data) const override;
    uint16_t MakeState(bool tip) const;
    int GetCrossTextureVariant(uint16_t data) const override;

private:
    bool IsTip(Level *level, const BlockPos &pos) const;

    int m_tipProperty;
    int m_variants[2];
};
