#include "ClimbableBlock.h"

ClimbableBlock::ClimbableBlock(const std::string &texturePath) : Block(false, texturePath)
{
    SetBlocksLiquid(true);
    SetHorizontalFacing();
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsAttachedSupport(true);
    SetClimbable(true);
    SetCollidable(true);
    SetFlatItem(true);
}

void ClimbableBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    Direction *wall = GetAttachment(data);
    glm::dvec3 min(0.0, 0.0, 0.0);
    glm::dvec3 max(1.0, 1.0, 1.0);
    if (wall == Direction::north)
    {
        max.z = kThickness;
    }
    else if (wall == Direction::south)
    {
        min.z = 1.0 - kThickness;
    }
    else if (wall == Direction::west)
    {
        max.x = kThickness;
    }
    else
    {
        min.x = 1.0 - kThickness;
    }

    out->push_back(AABB(min, max));
}

Block *ClimbableBlock::GetPlacementBlock(Direction *face)
{
    if (face == Direction::up || face == Direction::down)
    {
        return nullptr;
    }

    return this;
}
