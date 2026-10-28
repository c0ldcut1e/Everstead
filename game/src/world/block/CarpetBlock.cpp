#include "CarpetBlock.h"

#include "../Level.h"

CarpetBlock::CarpetBlock(const std::string &texturePath) : Block(false, texturePath)
{
    SetAABB(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 1.0 / 16.0, 1.0)));
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetBreakSound("dig.cloth");
    SetFlammable(60, 20);
}

bool CarpetBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;
    return level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)) != 0;
}
