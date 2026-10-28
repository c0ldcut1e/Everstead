#pragma once

#include "Block.h"

class HopperBlock : public Block
{
public:
    HopperBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    bool IsUsable(uint16_t data) const override;
    const char *GetBlockEntityId() const override;
    bool HasAnalogOutput() const override;
    int GetAnalogOutput(Level *level, const BlockPos &pos) const override;
    Direction *GetFacing(uint16_t data) const;
    bool IsEnabled(uint16_t data) const;

private:
    void UpdateEnabled(Level *level, const BlockPos &pos);

    int m_facingProperty;
    int m_enabledProperty;
    int m_insideTexture;
    int m_topTexture;
    int m_outsideTexture;
};
