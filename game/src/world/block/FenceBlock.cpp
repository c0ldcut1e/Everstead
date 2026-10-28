#include "FenceBlock.h"

#include "../Level.h"
#include "FenceGateBlock.h"
#include "WallBlock.h"

Direction *const FenceBlock::s_sides[4] = {Direction::north, Direction::east, Direction::south, Direction::west};

FenceBlock::FenceBlock(const std::string &texturePath) : Block(true, texturePath)
{
    m_sideProperties[0] = AddProperty("north", 2);
    m_sideProperties[1] = AddProperty("east", 2);
    m_sideProperties[2] = AddProperty("south", 2);
    m_sideProperties[3] = AddProperty("west", 2);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("dig.wood");
}

bool FenceBlock::ConnectsTo(const Block *block) const
{
    if (!block)
    {
        return false;
    }

    if ((dynamic_cast<const FenceBlock *>(block) && !dynamic_cast<const WallBlock *>(block)) || dynamic_cast<const FenceGateBlock *>(block))
    {
        return true;
    }

    return block->GetRenderShape() == eRenderShape_CUBE && block->IsSolid() && !block->IsLiquid() && !block->IsTransparent();
}

uint16_t FenceBlock::GetConnections(Level *level, const BlockPos &pos) const
{
    uint16_t data = 0;
    for (int side = 0; side < 4; side++)
    {
        Direction *direction = s_sides[side];
        Block *neighbor      = Block::ById(level->GetBlockId(BlockPos(pos.x + direction->dx, pos.y, pos.z + direction->dz)));
        data                 = WithPropertyValue(data, m_sideProperties[side], ConnectsTo(neighbor) ? 1 : 0);
    }

    return data;
}

void FenceBlock::UpdateShape(Level *level, const BlockPos &pos)
{
    uint16_t data = GetConnections(level, pos);
    if (data != level->GetBlockData(pos))
    {
        level->SetBlockData(pos, data);
        level->RecordBlockChange(pos);
    }
}

void FenceBlock::OnPlace(Level *level, const BlockPos &pos)
{
    UpdateShape(level, pos);
}

void FenceBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    UpdateShape(level, pos);
}

uint16_t FenceBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;
    (void) look;
    return GetConnections(level, pos);
}

uint16_t FenceBlock::GetItemData() const
{
    return WithPropertyValue(WithPropertyValue(0, m_sideProperties[0], 1), m_sideProperties[2], 1);
}

void FenceBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    out->push_back(AABB(glm::dvec3(kPostMin, 0.0, kPostMin), glm::dvec3(kPostMax, 1.0, kPostMax)));

    static const double s_barHeights[2][2] = {{0.375, 0.5625}, {0.75, 0.9375}};
    for (int side = 0; side < 4; side++)
    {
        if (GetPropertyValue(data, m_sideProperties[side]) == 0)
        {
            continue;
        }

        Direction *direction = s_sides[side];
        for (int bar = 0; bar < 2; bar++)
        {
            double minY = s_barHeights[bar][0];
            double maxY = s_barHeights[bar][1];
            if (direction->dx != 0)
            {
                double minX = direction->dx < 0 ? 0.0 : kPostMax;
                double maxX = direction->dx < 0 ? kPostMin : 1.0;
                out->push_back(AABB(glm::dvec3(minX, minY, kBarMin), glm::dvec3(maxX, maxY, kBarMax)));
            }
            else
            {
                double minZ = direction->dz < 0 ? 0.0 : kPostMax;
                double maxZ = direction->dz < 0 ? kPostMin : 1.0;
                out->push_back(AABB(glm::dvec3(kBarMin, minY, minZ), glm::dvec3(kBarMax, maxY, maxZ)));
            }
        }
    }
}

void FenceBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    out->push_back(AABB(glm::dvec3(kPostMin, 0.0, kPostMin), glm::dvec3(kPostMax, 1.5, kPostMax)));
    for (int side = 0; side < 4; side++)
    {
        if (GetPropertyValue(data, m_sideProperties[side]) == 0)
        {
            continue;
        }

        Direction *direction = s_sides[side];
        if (direction->dx != 0)
        {
            double minX = direction->dx < 0 ? 0.0 : kPostMax;
            double maxX = direction->dx < 0 ? kPostMin : 1.0;
            out->push_back(AABB(glm::dvec3(minX, 0.0, kPostMin), glm::dvec3(maxX, 1.5, kPostMax)));
        }
        else
        {
            double minZ = direction->dz < 0 ? 0.0 : kPostMax;
            double maxZ = direction->dz < 0 ? kPostMin : 1.0;
            out->push_back(AABB(glm::dvec3(kPostMin, 0.0, minZ), glm::dvec3(kPostMax, 1.5, maxZ)));
        }
    }
}
