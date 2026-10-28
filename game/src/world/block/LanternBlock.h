#pragma once

#include "Block.h"

class LanternBlock : public Block
{
public:
    LanternBlock(const std::string &name, uint8_t r, uint8_t g, uint8_t b);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const override;
    bool HasAmbientOcclusion() const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    uint16_t GetPlacementData(Direction *placedAgainst) const override;
    Direction *GetAttachment(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

    bool IsHanging(uint16_t data) const;

private:
    int m_hangingProperty;
};
