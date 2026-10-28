#pragma once

#include "Block.h"

class ResonatorBlock : public Block
{
public:
    ResonatorBlock();

    bool IsUsable(uint16_t data) const override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    const char *GetBlockEntityId() const override;
};
