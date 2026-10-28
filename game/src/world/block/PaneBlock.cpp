#include "PaneBlock.h"

#include "../Level.h"

static Direction *const s_paneSides[] = {Direction::north, Direction::east, Direction::south, Direction::west};

PaneBlock::PaneBlock(const std::string &texturePath, const std::string &edgeTexturePath) : Block(true, texturePath)
{
    m_sideProperties[0] = AddProperty("north", 2);
    m_sideProperties[1] = AddProperty("east", 2);
    m_sideProperties[2] = AddProperty("south", 2);
    m_sideProperties[3] = AddProperty("west", 2);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetSecondaryTexture(edgeTexturePath);
    SetBreakSound("random.glass", 3);
    SetPlaceSound("step.stone", 6, 0.8f);
}

bool PaneBlock::ConnectsTo(const Block *block)
{
    if (!block)
    {
        return false;
    }

    if (dynamic_cast<const PaneBlock *>(block))
    {
        return true;
    }

    if (block->GetRenderShape() != eRenderShape_CUBE || !block->IsSolid() || block->IsLiquid())
    {
        return false;
    }

    return !block->IsTransparent() || block->GetBreakSound() == "random.glass";
}

uint16_t PaneBlock::GetConnections(Level *level, const BlockPos &pos) const
{
    uint16_t data = 0;
    for (int side = 0; side < 4; side++)
    {
        Direction *direction = s_paneSides[side];
        Block *neighbor      = Block::ById(level->GetBlockId(BlockPos(pos.x + direction->dx, pos.y, pos.z + direction->dz)));
        data                 = WithPropertyValue(data, m_sideProperties[side], ConnectsTo(neighbor) ? 1 : 0);
    }

    return data;
}

void PaneBlock::UpdateShape(Level *level, const BlockPos &pos)
{
    uint16_t data = GetConnections(level, pos);
    if (data != level->GetBlockData(pos))
    {
        level->SetBlockData(pos, data);
        level->RecordBlockChange(pos);
    }
}

void PaneBlock::OnPlace(Level *level, const BlockPos &pos)
{
    UpdateShape(level, pos);
}

void PaneBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    UpdateShape(level, pos);
}

uint16_t PaneBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;
    (void) look;
    return GetConnections(level, pos);
}

void PaneBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    bool connected[4];
    bool any = false;
    for (int side = 0; side < 4; side++)
    {
        connected[side] = GetPropertyValue(data, m_sideProperties[side]) != 0;
        any             = any || connected[side];
    }

    if (!any)
    {
        for (int side = 0; side < 4; side++)
        {
            connected[side] = true;
        }
    }

    bool north = connected[0];
    bool east  = connected[1];
    bool south = connected[2];
    bool west  = connected[3];
    if (north || south)
    {
        out->push_back(AABB(glm::dvec3(kMin, 0.0, north ? 0.0 : kMin), glm::dvec3(kMax, 1.0, south ? 1.0 : kMax)));
    }

    if (west || east)
    {
        out->push_back(AABB(glm::dvec3(west ? 0.0 : kMin, 0.0, kMin), glm::dvec3(east ? 1.0 : kMax, 1.0, kMax)));
    }
}

Block::UVRect PaneBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) data;
    *flipU                = false;
    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();
    double faceWidth      = direction->dx != 0 ? max.z - min.z : max.x - min.x;
    if (direction == Direction::up || direction == Direction::down || faceWidth < 0.5)
    {
        return GetSecondaryAtlasUVRect();
    }

    return GetAtlasUVRect(direction);
}
