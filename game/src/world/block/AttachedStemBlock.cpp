#include "AttachedStemBlock.h"

#include "../Level.h"
#include "StemBlock.h"

AttachedStemBlock::AttachedStemBlock(const std::string &texturePath, const std::string &fruitName, StemBlock *stem) : Block(false, texturePath)
{
    m_fruitName = fruitName;
    m_stem      = stem;
    SetHorizontalFacing();
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetBreakSound("dig.wood");
}

void AttachedStemBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    if (!CanSurvive(level, pos, nullptr))
    {
        Block::OnNeighborChanged(level, pos);
        return;
    }

    Direction *facing = Direction::Opposite(GetAttachment(level->GetBlockData(pos)));
    if (Block::ById(level->GetBlockId(BlockPos(pos.x + facing->dx, pos.y, pos.z + facing->dz))) != Block::ByName(m_fruitName))
    {
        level->SetBlock(pos, m_stem, nullptr, m_stem->WithAge(7));
        level->RecordBlockChange(pos);
    }
}

bool AttachedStemBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    return m_stem->CanSurvive(level, pos, attachment);
}

void AttachedStemBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    Direction *facing = Direction::Opposite(GetAttachment(data));
    if (facing == Direction::west)
    {
        out->push_back(AABB(glm::dvec3(0.0, -1.0 / 16.0, 0.5), glm::dvec3(9.0 / 16.0, 15.0 / 16.0, 0.5)));
    }
    else if (facing == Direction::east)
    {
        out->push_back(AABB(glm::dvec3(7.0 / 16.0, -1.0 / 16.0, 0.5), glm::dvec3(1.0, 15.0 / 16.0, 0.5)));
    }
    else if (facing == Direction::north)
    {
        out->push_back(AABB(glm::dvec3(0.5, -1.0 / 16.0, 0.0), glm::dvec3(0.5, 15.0 / 16.0, 9.0 / 16.0)));
    }
    else
    {
        out->push_back(AABB(glm::dvec3(0.5, -1.0 / 16.0, 7.0 / 16.0), glm::dvec3(0.5, 15.0 / 16.0, 1.0)));
    }
}

Block::UVRect AttachedStemBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;

    Direction *facing = Direction::Opposite(GetAttachment(data));
    *flipU = (facing == Direction::west && direction == Direction::north) || (facing == Direction::east && direction == Direction::south) ||
             (facing == Direction::north && direction == Direction::east) || (facing == Direction::south && direction == Direction::west);
    return GetAtlasUVRect(direction);
}

void AttachedStemBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    Direction *facing = Direction::Opposite(GetAttachment(data));
    glm::dvec3 min(6.0 / 16.0, 0.0, 6.0 / 16.0);
    glm::dvec3 max(10.0 / 16.0, 10.0 / 16.0, 10.0 / 16.0);
    if (facing->dx < 0 || facing->dz < 0)
    {
        min = glm::dvec3(facing->dx < 0 ? 0.0 : min.x, 0.0, facing->dz < 0 ? 0.0 : min.z);
    }
    else
    {
        max = glm::dvec3(facing->dx > 0 ? 1.0 : max.x, max.y, facing->dz > 0 ? 1.0 : max.z);
    }

    out->push_back(AABB(min, max));
}

AABB AttachedStemBlock::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    std::vector<AABB> boxes;
    GetBoxes(data, &boxes);
    return boxes[0].Translated(glm::dvec3(pos.x, pos.y, pos.z));
}
