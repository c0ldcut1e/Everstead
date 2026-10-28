#pragma once

#include <string>

#include "Block.h"

class LiquidBlock : public Block
{
public:
    LiquidBlock(const std::string &texturePath, bool dynamic, bool lava);

    void SetCounterpart(LiquidBlock *counterpart);
    static float GetHeight(uint8_t depth);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;
    float GetLiquidHeight(uint8_t data) const override;
    glm::dvec3 GetFlow(Level *level, const BlockPos &pos) const override;
    bool IsLava() const override;

private:
    bool IsLiquidBlock(Block *block) const;
    int GetDepth(Level *level, const BlockPos &pos) const;
    int GetRenderedDepth(Level *level, const BlockPos &pos) const;
    int GetHighestDepth(Level *level, const BlockPos &pos, int currentDepth, int *sourceCount) const;
    bool HasLoadedBlock(Level *level, const BlockPos &pos) const;
    bool IsLiquidBlocking(Level *level, const BlockPos &pos) const;
    bool CanSpreadTo(Level *level, const BlockPos &pos) const;
    int GetSlopeDistance(Level *level, const BlockPos &pos, int pass, int from) const;
    void GetSpread(Level *level, const BlockPos &pos, bool *outSpread) const;
    void SetFlowing(Level *level, const BlockPos &pos, uint8_t depth);
    void SetStill(Level *level, const BlockPos &pos);
    void TrySpreadTo(Level *level, const BlockPos &pos, uint8_t depth);
    int GetTickDelay() const;
    bool IsWaterAt(Level *level, const BlockPos &pos) const;
    bool HardenLava(Level *level, const BlockPos &pos);
    void Fizz(Level *level, const BlockPos &pos);
    bool IsFlammableAt(Level *level, const BlockPos &pos) const;
    bool HasFlammableNeighbor(Level *level, const BlockPos &pos) const;

    bool m_dynamic;
    bool m_lava;
    LiquidBlock *m_counterpart;
};
