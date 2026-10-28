#pragma once

#include "Block.h"

class BrittleIceBlock : public Block
{
public:
    BrittleIceBlock();

    void Tick(Level *level, const BlockPos &pos) override;
    UVRect GetAtlasUVRect(Direction *direction, uint16_t data) const override;
    void OnSteppedOn(Level *level, const BlockPos &pos);

private:
    static constexpr int kStageCount = 4;
    static constexpr int kStageTicks = 20;

    int m_stageProperty;
};
