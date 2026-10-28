#include "CraftingTableBlock.h"

CraftingTableBlock::CraftingTableBlock() : Block(true, "assets/texture/block/crafting_table_side.png")
{}

bool CraftingTableBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}
