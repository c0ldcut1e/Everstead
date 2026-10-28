#pragma once

#include "RailBlock.h"

class PoweredRailBlock : public RailBlock
{
public:
    PoweredRailBlock(const std::string &texturePath, const std::string &poweredTexturePath);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;

private:
    void UpdatePower(Level *level, const BlockPos &pos);
    bool FindPoweredSignal(Level *level, BlockPos pos, uint16_t data, bool forward, int depth) const;
    bool IsPoweredRailWithSignal(Level *level, const BlockPos &pos, bool forward, int depth, int axisShape) const;
};
