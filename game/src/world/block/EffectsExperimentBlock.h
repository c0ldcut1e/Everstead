#pragma once

#include <unordered_map>

#include "../../utils/Random.h"
#include "Block.h"

class EffectsExperimentBlock : public Block
{
public:
    EffectsExperimentBlock();

    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;

private:
    std::unordered_map<BlockPos, int> m_stages;
};
