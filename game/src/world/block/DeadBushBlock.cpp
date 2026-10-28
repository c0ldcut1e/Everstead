#include "DeadBushBlock.h"

#include "../Level.h"

DeadBushBlock::DeadBushBlock(const std::string &texturePath) : Block(false, texturePath)
{
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetAABB(AABB(glm::dvec3(0.1, 0.0, 0.1), glm::dvec3(0.9, 0.8, 0.9)));
    SetReplaceable(true);
    SetNeedsSupportBelow(true);
}

bool DeadBushBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == Block::ByName("sand") || below == Block::ByName("red_sand") || below == Block::ByName("dirt") ||
           below == Block::ByName("grass_block");
}
