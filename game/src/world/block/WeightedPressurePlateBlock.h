#pragma once

#include "Block.h"

class WeightedPressurePlateBlock : public Block
{
public:
    WeightedPressurePlateBlock(const std::string &texturePath, int maxWeight);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void Tick(Level *level, const BlockPos &pos) override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;

    void OnEntityInside(Level *level, const BlockPos &pos);

private:
    int GetSignalStrength(Level *level, const BlockPos &pos) const;
    void UpdateSignal(Level *level, const BlockPos &pos);
    void NotifySignalNeighbors(Level *level, const BlockPos &pos);

    int m_maxWeight;
    int m_powerProperty;
};
