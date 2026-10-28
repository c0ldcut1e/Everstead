#pragma once

#include "DispenserBlock.h"

class DropperBlock : public DispenserBlock
{
public:
    DropperBlock();

    const char *GetBlockEntityId() const override;

protected:
    void DispenseFrom(Level *level, const BlockPos &pos, DispenserBlockEntity *dispenser, int slot) override;
};
