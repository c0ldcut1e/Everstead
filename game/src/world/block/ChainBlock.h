#pragma once

#include "Block.h"

class ChainBlock : public Block
{
public:
    ChainBlock();

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const override;
    bool HasAmbientOcclusion() const override;
    uint16_t GetPlacementData(Direction *placedAgainst) const override;

    int GetAxis(uint16_t data) const;

private:
    int m_chainAxisProperty;
};
