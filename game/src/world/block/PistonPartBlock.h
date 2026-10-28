#pragma once

#include <string>

#include "Block.h"

class PistonPartBlock : public Block
{
public:
    PistonPartBlock(const std::string &texturePath);

    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    Direction *GetFacing(uint16_t data) const;
    uint16_t WithFacing(uint16_t data, Direction *facing) const;

protected:
    virtual int GetEndTexture(uint16_t data, bool front) const = 0;
    AABB MakeBox(Direction *facing, double start, double end, double halfWidth) const;

    int m_facingProperty;
    int m_topTexture;
    int m_stickyTopTexture;
    int m_innerTexture;
    int m_bottomTexture;
    int m_sideTexture;
    int m_rotatedSideTexture;

private:
    bool IsArm(const AABB &box, Direction *facing) const;
    void GetFaceAxes(Direction *direction, Direction *facing, int *alongU, int *alongV) const;
};
