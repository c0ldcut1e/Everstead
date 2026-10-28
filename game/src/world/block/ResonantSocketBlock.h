#pragma once

#include "Block.h"

class ResonantSocketBlock : public Block
{
public:
    ResonantSocketBlock();

    bool IsUsable(uint16_t data) const override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    const char *GetBlockEntityId() const override;
};
