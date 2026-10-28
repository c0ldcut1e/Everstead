#pragma once

#include <string>

#include "Block.h"

class CropBlock : public Block
{
public:
    CropBlock(const std::string &texturePrefix, int stageCount, int maxAge, const std::string &seedName);

    void RandomTick(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    bool CanPlaceAt(Level *level, const BlockPos &pos) const;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const override;
    AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const override;
    bool HasDirectionalShade() const override;
    int GetMaxAge() const;
    uint16_t GetAgeData(int age) const;
    bool CanBoneMeal(uint16_t data) const;
    void BoneMeal(Level *level, const BlockPos &pos) const;
    static float GetGrowthSpeed(Level *level, const BlockPos &pos, const Block *crop);

private:
    int GetStage(uint16_t data) const;

    int m_ageProperty;
    int m_maxAge;
    int m_stageCount;
    std::string m_seedName;
};
