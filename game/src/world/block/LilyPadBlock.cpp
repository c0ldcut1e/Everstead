#include "LilyPadBlock.h"

#include "../Level.h"

LilyPadBlock::LilyPadBlock() : Block(false, "assets/texture/block/lily_pad.png")
{
    SetAABB(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 1.5 / 16.0, 1.0)));
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetFixedTint(0x208030);
    SetFlatItem(true);
    SetBreakSound("dig.grass");
}

bool LilyPadBlock::HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;
    (void) boxIndex;
    return direction == Direction::up || direction == Direction::down;
}

bool LilyPadBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;
    BlockPos below(pos.x, pos.y - 1, pos.z);
    Block *water = Block::ById(level->GetBlockId(below));
    // A neighbor update briefly turns still water into a flowing source, so depth 0 flowing water still counts
    return water == Block::ByName("water_still") || (water == Block::ByName("water_flow") && level->GetBlockData(below) == 0);
}
