#include "PinkPetalsBlock.h"

#include "../Level.h"

PinkPetalsBlock::PinkPetalsBlock(const std::string &texturePath) : CarpetBlock(texturePath)
{
    SetCollidable(false);
    SetFlatItem(true);
    SetBreakSound("dig.grass");
}

bool PinkPetalsBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == Block::ByName("grass_block") || below == Block::ByName("dirt") || below == Block::ByName("coarse_dirt") ||
           below == Block::ByName("podzol") || below == Block::ByName("farmland");
}
