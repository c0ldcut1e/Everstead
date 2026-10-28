#include "ScaffoldingBlock.h"

#include <algorithm>

#include "../Level.h"

ScaffoldingBlock::ScaffoldingBlock() : Block(false, "assets/texture/block/scaffolding_side.png")
{
    SetBlocksLiquid(true);
    m_distanceProperty = AddProperty("distance", 8);
    m_bottomProperty   = AddProperty("bottom", 2);
    SetFaceTexture(Direction::up, "assets/texture/block/scaffolding_top.png");
    SetFaceTexture(Direction::down, "assets/texture/block/scaffolding_bottom.png");
    SetAABB(AABB(glm::dvec3(0.0), glm::dvec3(1.0)));
    SetTransparent(true);
    SetCollidable(false);
    SetClimbable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("scaffolding.place", 4);
    SetPlaceSound("scaffolding.place", 4, 0.8f);
}

// We keep the box order fixed because HasBoxFace and GetBoxFaceAtlasUVRect pick faces by index
void ScaffoldingBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    out->push_back(AABB(glm::dvec3(0.0, 15.99 * p, 0.0), glm::dvec3(1.0, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(2 * p, 1.0, 2 * p)));
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 14 * p), glm::dvec3(2 * p, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(14 * p, 0.0, 14 * p), glm::dvec3(1.0, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(14 * p, 0.0, 0.0), glm::dvec3(1.0, 1.0, 2 * p)));
    out->push_back(AABB(glm::dvec3(2 * p, 14 * p, 0.0), glm::dvec3(14 * p, 1.0, 2 * p)));
    out->push_back(AABB(glm::dvec3(2 * p, 14 * p, 14 * p), glm::dvec3(14 * p, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(14 * p, 14 * p, 2 * p), glm::dvec3(1.0, 1.0, 14 * p)));
    out->push_back(AABB(glm::dvec3(0.0, 14 * p, 2 * p), glm::dvec3(2 * p, 1.0, 14 * p)));
    if (IsBottom(data))
    {
        out->push_back(AABB(glm::dvec3(2 * p, 0.0, 0.0), glm::dvec3(14 * p, 2 * p, 2 * p)));
        out->push_back(AABB(glm::dvec3(2 * p, 0.0, 14 * p), glm::dvec3(14 * p, 2 * p, 1.0)));
        out->push_back(AABB(glm::dvec3(14 * p, 0.0, 2 * p), glm::dvec3(1.0, 2 * p, 14 * p)));
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 2 * p), glm::dvec3(2 * p, 2 * p, 14 * p)));
    }
}

bool ScaffoldingBlock::HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;

    return boxIndex == 0 || boxIndex > 8 || direction != Direction::up;
}

Block::UVRect ScaffoldingBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) data;

    *flipU = false;
    if (box.GetMin().y > 0.5 && box.GetMax().y - box.GetMin().y < 0.01)
    {
        return GetAtlasUVRect(Direction::up);
    }

    return GetAtlasUVRect(direction == Direction::up ? Direction::down : direction);
}

bool ScaffoldingBlock::HasAmbientOcclusion() const
{
    return false;
}

int ScaffoldingBlock::GetDistance(Level *level, const BlockPos &pos) const
{
    const int maxDistance = 7;

    BlockPos below(pos.x, pos.y - 1, pos.z);
    Block *belowBlock = Block::ById(level->GetBlockId(below));
    int distance      = maxDistance;
    if (belowBlock == this)
    {
        distance = GetPropertyValue(level->GetBlockData(below), m_distanceProperty);
    }
    else if (belowBlock->IsFaceSturdy(level->GetBlockData(below), Direction::up))
    {
        return 0;
    }

    for (int id = 1; id <= 6; id++)
    {
        Direction *side = Direction::ById(id);
        BlockPos neighbor(pos.x + side->dx, pos.y, pos.z + side->dz);
        if (side->dy == 0 && Block::ById(level->GetBlockId(neighbor)) == this)
        {
            distance = std::min(distance, GetPropertyValue(level->GetBlockData(neighbor), m_distanceProperty) + 1);
        }
    }

    return distance;
}

bool ScaffoldingBlock::IsBottom(uint16_t data) const
{
    return GetPropertyValue(data, m_bottomProperty) != 0;
}

bool ScaffoldingBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    return GetDistance(level, pos) < 7;
}

void ScaffoldingBlock::OnPlace(Level *level, const BlockPos &pos)
{
    OnNeighborChanged(level, pos);
}

void ScaffoldingBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide())
    {
        level->ScheduleBlockForTick(pos, 1, 0);
    }
}

void ScaffoldingBlock::Tick(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    int distance  = GetDistance(level, pos);
    if (distance >= 7)
    {
        DropAsItem(level, pos, data);
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
        return;
    }

    bool bottom      = distance > 0 && Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) != this;
    uint16_t newData = WithPropertyValue(WithPropertyValue(data, m_distanceProperty, distance), m_bottomProperty, bottom ? 1 : 0);
    if (newData != data)
    {
        level->SetBlockData(pos, newData);
        level->RecordBlockChange(pos);
        level->NotifyBlockNeighbors(pos);
    }
}
