#include "TrapDoorBlock.h"

#include <cmath>

#include "../Level.h"

TrapDoorBlock::TrapDoorBlock(const std::string &texturePath) : OpenableBlock(false, texturePath)
{
    SetBlocksLiquid(true);
    SetHorizontalFacing();
    m_halfProperty    = AddProperty("half", 2);
    m_openProperty    = AddProperty("open", 2);
    m_poweredProperty = AddProperty("powered", 2);
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("dig.wood");
}

void TrapDoorBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double thickness = 3.0 / 16.0;
    glm::dvec3 min(0.0, 0.0, 0.0);
    glm::dvec3 max(1.0, 1.0, 1.0);
    if (!IsOpen(data))
    {
        if (GetPropertyValue(data, m_halfProperty) != 0)
        {
            min.y = 1.0 - thickness;
        }
        else
        {
            max.y = thickness;
        }
    }
    else
    {
        Direction *wall = GetAttachment(data);
        if (wall == Direction::north)
        {
            max.z = thickness;
        }
        else if (wall == Direction::south)
        {
            min.z = 1.0 - thickness;
        }
        else if (wall == Direction::west)
        {
            max.x = thickness;
        }
        else
        {
            min.x = 1.0 - thickness;
        }
    }

    out->push_back(AABB(min, max));
}

uint16_t TrapDoorBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    if (face->dy == 0)
    {
        return WithPlacementPower(level, pos, WithPropertyValue(GetPlacementData(face), m_halfProperty, hitPos.y - (double) pos.y > 0.5 ? 1 : 0));
    }

    Direction *facing = Direction::south;
    if (std::abs(look.x) > std::abs(look.z))
    {
        facing = look.x > 0.0 ? Direction::west : Direction::east;
    }
    else if (look.z > 0.0)
    {
        facing = Direction::north;
    }

    return WithPlacementPower(level, pos, WithPropertyValue(GetPlacementData(facing), m_halfProperty, face == Direction::down ? 1 : 0));
}
