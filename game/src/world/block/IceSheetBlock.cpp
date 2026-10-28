#include "IceSheetBlock.h"

#include "../Level.h"

IceSheetBlock::IceSheetBlock(const std::string &texturePath) : Block(false, texturePath)
{
    for (int i = 0; i < kFaceCount; i++)
    {
        m_faceProperties[i] = AddProperty(Direction::ById(i + 1)->name, 2);
    }

    SetTransparent(true);
    SetTranslucent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetReplaceable(true);
    SetFlatItem(true);
    SetSlipperiness(0.98f);
}

bool IceSheetBlock::CanAttachTo(const Block *block)
{
    return block && block->IsSolid() && !block->IsLiquid() && block->GetRenderShape() == eRenderShape_CUBE;
}

bool IceSheetBlock::HasFace(uint16_t data, Direction *side) const
{
    return GetPropertyValue(data, m_faceProperties[side->GetId() - 1]) != 0;
}

uint16_t IceSheetBlock::WithFace(uint16_t data, Direction *side) const
{
    return WithPropertyValue(data, m_faceProperties[side->GetId() - 1], 1);
}

void IceSheetBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    for (int i = 0; i < kFaceCount; i++)
    {
        if (GetPropertyValue(data, m_faceProperties[i]) == 0)
        {
            continue;
        }

        Direction *side = Direction::ById(i + 1);
        glm::dvec3 min(side->dx > 0 ? 1.0 - kThickness : 0.0, side->dy > 0 ? 1.0 - kThickness : 0.0, side->dz > 0 ? 1.0 - kThickness : 0.0);
        glm::dvec3 max(side->dx < 0 ? kThickness : 1.0, side->dy < 0 ? kThickness : 1.0, side->dz < 0 ? kThickness : 1.0);
        out->push_back(AABB(min, max));
    }

    if (out->empty())
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, kThickness, 1.0)));
    }
}

void IceSheetBlock::GetSelectionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const
{
    (void) level;
    GetBoxes(data, out);
    for (size_t i = 0; i < out->size(); i++)
    {
        (*out)[i] = (*out)[i].Translated(glm::dvec3(pos.x, pos.y, pos.z));
    }
}

void IceSheetBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    (void) out;
}

uint16_t IceSheetBlock::GetPlacementData(Direction *placedAgainst) const
{
    return placedAgainst ? WithFace(0, Direction::Opposite(placedAgainst)) : WithFace(0, Direction::down);
}

uint16_t IceSheetBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) hitPos;
    (void) look;

    uint16_t existing = Block::ById(level->GetBlockId(pos)) == this ? level->GetBlockData(pos) : 0;
    if (!face)
    {
        return existing;
    }

    Direction *side = Direction::Opposite(face);
    if (!CanAttachTo(Block::ById(level->GetBlockId(BlockPos(pos.x + side->dx, pos.y + side->dy, pos.z + side->dz)))))
    {
        return existing;
    }

    return WithFace(existing, side);
}

Block *IceSheetBlock::GetCombinedBlock(uint16_t existingData, uint16_t placedData) const
{
    if (placedData == existingData || (placedData & existingData) != existingData)
    {
        return nullptr;
    }

    return (Block *) this;
}

Direction *IceSheetBlock::GetAttachment(uint16_t data) const
{
    for (int i = 0; i < kFaceCount; i++)
    {
        if (GetPropertyValue(data, m_faceProperties[i]) != 0)
        {
            return Direction::ById(i + 1);
        }
    }

    return Direction::down;
}

bool IceSheetBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    if (Block::ById(level->GetBlockId(pos)) == this)
    {
        return GetSurvivingFaces(level, pos, level->GetBlockData(pos)) != 0;
    }

    return attachment &&
           CanAttachTo(Block::ById(level->GetBlockId(BlockPos(pos.x + attachment->dx, pos.y + attachment->dy, pos.z + attachment->dz))));
}

uint16_t IceSheetBlock::GetSurvivingFaces(Level *level, const BlockPos &pos, uint16_t data) const
{
    uint16_t result = 0;
    for (int i = 0; i < kFaceCount; i++)
    {
        Direction *side = Direction::ById(i + 1);
        if (HasFace(data, side) && CanAttachTo(Block::ById(level->GetBlockId(BlockPos(pos.x + side->dx, pos.y + side->dy, pos.z + side->dz)))))
        {
            result = WithFace(result, side);
        }
    }

    return result;
}

void IceSheetBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data      = level->GetBlockData(pos);
    uint16_t surviving = GetSurvivingFaces(level, pos, data);
    if (surviving == data)
    {
        return;
    }

    if (surviving == 0)
    {
        level->SetBlock(pos, Block::ByName("air"));
    }
    else
    {
        level->SetBlockData(pos, surviving);
    }

    level->RecordBlockChange(pos);
}
