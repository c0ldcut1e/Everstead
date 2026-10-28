#pragma once

#include "OpenableBlock.h"

class DoorBlock : public OpenableBlock
{
public:
    DoorBlock(const std::string &lowerTexturePath, const std::string &upperTexturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnClientPlace(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    Block *GetPlacementBlock(Direction *face) override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    BlockPos GetOtherPos(const BlockPos &pos, uint16_t data) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;

private:
    bool IsFaceMirrored(uint16_t data, Direction *direction) const;
    static constexpr float kThickness = 0.1875f;

    bool IsUpper(uint16_t data) const;
    bool IsHingeRight(uint16_t data) const;
    int GetFacing(uint16_t data) const;
    int CountSolid(Level *level, const BlockPos &pos) const;
    bool HasSturdyTop(Level *level, const BlockPos &pos) const;
    bool IsDoorAt(Level *level, const BlockPos &pos) const;

    int m_facingProperty;
    int m_halfProperty;
    int m_hingeProperty;
};
