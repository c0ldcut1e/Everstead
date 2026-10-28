#pragma once

#include "Block.h"

class CocoaBlock : public Block
{
public:
    CocoaBlock();

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    Block *GetPlacementBlock(Direction *face) override;
    int GetAge(uint16_t data) const;
    uint16_t WithAge(uint16_t data, int age) const;

private:
    int m_ageProperty;
};
