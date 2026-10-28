#pragma once

#include "Block.h"

class BambooBlock : public Block
{
public:
    enum eLeaves
    {
        eLeaves_None,
        eLeaves_Small,
        eLeaves_Large,
    };

    BambooBlock();

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    void Grow(Level *level, const BlockPos &pos, int height) const;
    int GetAge(uint16_t data) const;
    uint16_t GetStateData(int age, eLeaves leaves, int stage) const;
    static bool CanPlantOn(const Block *block);

private:
    int m_ageProperty;
    int m_leavesProperty;
    int m_stageProperty;
    int m_smallLeavesTexture;
    int m_largeLeavesTexture;
};
