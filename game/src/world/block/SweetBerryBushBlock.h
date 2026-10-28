#pragma once

#include "Block.h"

class SweetBerryBushBlock : public Block
{
public:
    SweetBerryBushBlock();

    void RandomTick(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    UVRect GetCrossAtlasUVRect(uint16_t data) const override;
    int GetCrossTextureVariant(uint16_t data) const override;
    AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const override;
    uint16_t GetAgeData(int age) const;
    bool CanBoneMeal(uint16_t data) const;
    void BoneMeal(Level *level, const BlockPos &pos) const;

    static constexpr int kMaxAge = 3;

private:
    int m_ageProperty;
};
