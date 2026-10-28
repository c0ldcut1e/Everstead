#include "StairBlock.h"

#include <cmath>

#include "../Level.h"

StairBlock::StairBlock(const std::string &texturePath) : Block(true, texturePath)
{
    m_facingProperty = AddProperty("facing", 4);
    m_halfProperty   = AddProperty("half", 2);
    m_shapeProperty  = AddProperty("shape", 5);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
}

Direction *StairBlock::FacingDirection(int facing)
{
    static Direction *directions[] = {Direction::north, Direction::east, Direction::south, Direction::west};
    return directions[facing & 3];
}

bool StairBlock::InHalf(int quadrantX, int quadrantZ, int facing)
{
    Direction *direction = FacingDirection(facing);
    if (direction->dx != 0)
    {
        return quadrantX == (direction->dx > 0 ? 1 : 0);
    }

    return quadrantZ == (direction->dz > 0 ? 1 : 0);
}

int StairBlock::GetFacing(uint16_t data) const
{
    return GetPropertyValue(data, m_facingProperty);
}

bool StairBlock::IsTopHalf(uint16_t data) const
{
    return GetPropertyValue(data, m_halfProperty) == 1;
}

void StairBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    int facing = GetFacing(data);
    int shape  = GetPropertyValue(data, m_shapeProperty);
    bool top   = IsTopHalf(data);
    int ccw    = (facing + 3) & 3;
    int cw     = (facing + 1) & 3;
    int back   = (facing + 2) & 3;

    bool quadrants[2][2];
    for (int qx = 0; qx < 2; qx++)
    {
        for (int qz = 0; qz < 2; qz++)
        {
            bool front = InHalf(qx, qz, facing);
            if (shape == eStairShape_OuterLeft)
            {
                quadrants[qx][qz] = front && InHalf(qx, qz, ccw);
            }
            else if (shape == eStairShape_OuterRight)
            {
                quadrants[qx][qz] = front && InHalf(qx, qz, cw);
            }
            else if (shape == eStairShape_InnerLeft)
            {
                quadrants[qx][qz] = front || (InHalf(qx, qz, back) && InHalf(qx, qz, ccw));
            }
            else if (shape == eStairShape_InnerRight)
            {
                quadrants[qx][qz] = front || (InHalf(qx, qz, back) && InHalf(qx, qz, cw));
            }
            else
            {
                quadrants[qx][qz] = front;
            }
        }
    }

    double slabMinY = top ? 0.5 : 0.0;
    double stepMinY = top ? 0.0 : 0.5;
    out->push_back(AABB(glm::dvec3(0.0, slabMinY, 0.0), glm::dvec3(1.0, slabMinY + 0.5, 1.0)));

    for (int side = 0; side < 4; side++)
    {
        int count = 0;
        for (int qx = 0; qx < 2; qx++)
        {
            for (int qz = 0; qz < 2; qz++)
            {
                count += quadrants[qx][qz] && InHalf(qx, qz, side) ? 1 : 0;
            }
        }

        if (count != 2)
        {
            continue;
        }

        Direction *direction = FacingDirection(side);
        double minX          = direction->dx > 0 ? 0.5 : 0.0;
        double maxX          = direction->dx < 0 ? 0.5 : 1.0;
        double minZ          = direction->dz > 0 ? 0.5 : 0.0;
        double maxZ          = direction->dz < 0 ? 0.5 : 1.0;
        out->push_back(AABB(glm::dvec3(minX, stepMinY, minZ), glm::dvec3(maxX, stepMinY + 0.5, maxZ)));
        for (int qx = 0; qx < 2; qx++)
        {
            for (int qz = 0; qz < 2; qz++)
            {
                if (InHalf(qx, qz, side))
                {
                    quadrants[qx][qz] = false;
                }
            }
        }
    }

    for (int qx = 0; qx < 2; qx++)
    {
        for (int qz = 0; qz < 2; qz++)
        {
            if (quadrants[qx][qz])
            {
                out->push_back(AABB(glm::dvec3(qx * 0.5, stepMinY, qz * 0.5), glm::dvec3(qx * 0.5 + 0.5, stepMinY + 0.5, qz * 0.5 + 0.5)));
            }
        }
    }
}

bool StairBlock::GetNeighborStairs(Level *level, const BlockPos &pos, int facing, uint16_t *outData) const
{
    Direction *direction = FacingDirection(facing);
    BlockPos neighbor(pos.x + direction->dx, pos.y, pos.z + direction->dz);
    if (!dynamic_cast<StairBlock *>(Block::ById(level->GetBlockId(neighbor))))
    {
        return false;
    }

    *outData = level->GetBlockData(neighbor);
    return true;
}

bool StairBlock::CanTakeShape(Level *level, const BlockPos &pos, uint16_t data, int face) const
{
    uint16_t neighborData = 0;
    if (!GetNeighborStairs(level, pos, face, &neighborData))
    {
        return true;
    }

    return GetFacing(neighborData) != GetFacing(data) || IsTopHalf(neighborData) != IsTopHalf(data);
}

uint16_t StairBlock::WithShape(Level *level, const BlockPos &pos, uint16_t data) const
{
    int facing = GetFacing(data);
    int ccw    = (facing + 3) & 3;
    int shape  = eStairShape_Straight;

    uint16_t neighborData = 0;
    if (GetNeighborStairs(level, pos, facing, &neighborData) && IsTopHalf(neighborData) == IsTopHalf(data))
    {
        int neighborFacing = GetFacing(neighborData);
        if ((neighborFacing & 1) != (facing & 1) && CanTakeShape(level, pos, data, (neighborFacing + 2) & 3))
        {
            shape = neighborFacing == ccw ? eStairShape_OuterLeft : eStairShape_OuterRight;
        }
    }

    if (shape == eStairShape_Straight && GetNeighborStairs(level, pos, (facing + 2) & 3, &neighborData) && IsTopHalf(neighborData) == IsTopHalf(data))
    {
        int neighborFacing = GetFacing(neighborData);
        if ((neighborFacing & 1) != (facing & 1) && CanTakeShape(level, pos, data, neighborFacing))
        {
            shape = neighborFacing == ccw ? eStairShape_InnerLeft : eStairShape_InnerRight;
        }
    }

    return WithPropertyValue(data, m_shapeProperty, shape);
}

uint16_t StairBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    int facing = 0;
    if (fabs(look.x) > fabs(look.z))
    {
        facing = look.x > 0.0 ? 1 : 3;
    }
    else
    {
        facing = look.z > 0.0 ? 2 : 0;
    }

    bool top      = face == Direction::down || (face != Direction::up && hitPos.y - (double) pos.y > 0.5);
    uint16_t data = WithPropertyValue(0, m_facingProperty, facing);
    data          = WithPropertyValue(data, m_halfProperty, top ? 1 : 0);
    return WithShape(level, pos, data);
}

uint16_t StairBlock::GetItemData() const
{
    return WithPropertyValue(0, m_facingProperty, 3);
}

void StairBlock::UpdateShape(Level *level, const BlockPos &pos)
{
    uint16_t data    = level->GetBlockData(pos);
    uint16_t updated = WithShape(level, pos, data);
    if (updated != data)
    {
        level->SetBlockData(pos, updated);
        level->RecordBlockChange(pos);
    }
}

void StairBlock::OnPlace(Level *level, const BlockPos &pos)
{
    Block::OnPlace(level, pos);
    UpdateShape(level, pos);
}

void StairBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    UpdateShape(level, pos);
}
