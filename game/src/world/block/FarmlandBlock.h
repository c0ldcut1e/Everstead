#pragma once

#include "Block.h"

class FarmlandBlock : public Block
{
public:
    FarmlandBlock(const std::string &texturePath, const std::string &dryTopPath, const std::string &wetTopPath);

    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void OnFallOn(Level *level, const BlockPos &pos, float fallDistance);

private:
    static constexpr int kMaxMoisture = 7;

    int m_moistureProperty;

    static bool IsNearWater(Level *level, const BlockPos &pos);
};
