#pragma once

#include "RailBlock.h"

class DetectorRailBlock : public RailBlock
{
public:
    DetectorRailBlock(const std::string &texturePath, const std::string &poweredTexturePath);

    void Tick(Level *level, const BlockPos &pos) override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;

    void OnMinecartInside(Level *level, const BlockPos &pos);

private:
    void CheckPressed(Level *level, const BlockPos &pos);
};
