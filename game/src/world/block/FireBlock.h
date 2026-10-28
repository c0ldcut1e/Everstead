#pragma once

#include "Block.h"

class FireBlock : public Block
{
public:
    FireBlock(const std::string &texturePath, const std::string &secondaryTexturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

    bool IsBurnable(Level *level, const BlockPos &pos) const;

private:
    static constexpr int kMaxAge             = 15;
    static constexpr int kTickDelay          = 30;
    static constexpr int kTickDelayRange     = 10;
    static constexpr int kSideBurnChance     = 300;
    static constexpr int kVerticalBurnChance = 250;

    int GetEncouragement(Level *level, const BlockPos &pos) const;
    int GetNeighborEncouragement(Level *level, const BlockPos &pos) const;
    bool HasBurnableNeighbor(Level *level, const BlockPos &pos) const;
    bool IsRainingAround(Level *level, const BlockPos &pos) const;
    void TryCatchFire(Level *level, const BlockPos &pos, int chance, int age);
    void Extinguish(Level *level, const BlockPos &pos);

    int m_ageProperty;
};
