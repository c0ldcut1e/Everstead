#pragma once

#include "Block.h"

class BedBlock : public Block
{
public:
    BedBlock(const std::string &color);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnClientPlace(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    bool HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const override;
    bool IsBoxFaceDoubleSided(uint16_t data, size_t boxIndex, Direction *direction) const override;
    Block *GetPlacementBlock(Direction *face) override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    bool IsUsable(uint16_t data) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;

    bool CanPlaceAt(Level *level, const BlockPos &pos, uint16_t data) const;
    int GetFacing(uint16_t data) const;
    bool IsHead(uint16_t data) const;
    bool IsOccupied(uint16_t data) const;
    void SetOccupied(Level *level, const BlockPos &pos, bool occupied);
    BlockPos GetHeadPos(const BlockPos &pos, uint16_t data) const;
    BlockPos GetOtherPos(const BlockPos &pos, uint16_t data) const override;
    Direction *GetHeadDirection(uint16_t data) const;
    bool FindStandUpPosition(Level *level, const BlockPos &pos, BlockPos *out) const;

private:
    bool IsBedAt(Level *level, const BlockPos &pos) const;
    bool IsFreeWithSupport(Level *level, const BlockPos &pos) const;

    int m_facingProperty;
    int m_headProperty;
    int m_occupiedProperty;
    int m_headTop;
    int m_footTop;
    int m_headTopTransposed;
    int m_footTopTransposed;
    int m_headEnd;
    int m_footEnd;
    int m_headSide;
    int m_footSide;
    int m_bottom;
};
