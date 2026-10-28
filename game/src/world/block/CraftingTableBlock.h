#pragma once

#include "Block.h"

class CraftingTableBlock : public Block
{
public:
    CraftingTableBlock();

    bool IsUsable(uint16_t data) const override;
};
