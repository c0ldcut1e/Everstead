#include "CactusBlock.h"

#include "../Level.h"

CactusBlock::CactusBlock(const std::string &sidePath, const std::string &topPath, const std::string &bottomPath) : Block(true, sidePath)
{
    SetFaceTexture(Direction::up, topPath);
    SetFaceTexture(Direction::down, bottomPath);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetAABB(AABB(glm::dvec3(kInset, 0.0, kInset), glm::dvec3(1.0 - kInset, 1.0, 1.0 - kInset)));
    SetRandomTicking(true);
    m_ageProperty = AddProperty("age", 16);
}

void CactusBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(kInset, 0.0, kInset), glm::dvec3(1.0 - kInset, 1.0, 1.0 - kInset)));
}

void CactusBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(kInset, 0.0, kInset), glm::dvec3(1.0 - kInset, 1.0 - kInset, 1.0 - kInset)));
}

void CactusBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0, 0.0, kInset), glm::dvec3(1.0, 1.0, 1.0 - kInset)));
    out->push_back(AABB(glm::dvec3(kInset, 0.0, 0.0), glm::dvec3(1.0 - kInset, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 1.0, 1.0)));
}

bool CactusBlock::HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;

    if (boxIndex == 0)
    {
        return direction == Direction::north || direction == Direction::south;
    }

    if (boxIndex == 1)
    {
        return direction == Direction::east || direction == Direction::west;
    }

    return direction == Direction::up || direction == Direction::down;
}

bool CactusBlock::IsBoxFaceDoubleSided(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;
    (void) direction;

    // The spike boxes stick out past the side panels, so their back faces are visible
    return boxIndex < 2;
}

bool CactusBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Direction *sides[] = {Direction::north, Direction::south, Direction::west, Direction::east};
    for (int i = 0; i < 4; i++)
    {
        Block *side = Block::ById(level->GetBlockId(BlockPos(pos.x + sides[i]->dx, pos.y, pos.z + sides[i]->dz)));
        if (side && side->IsSolid())
        {
            return false;
        }
    }

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == this || below == Block::ByName("sand") || below == Block::ByName("red_sand");
}

void CactusBlock::RandomTick(Level *level, const BlockPos &pos)
{
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (level->IsClientSide() || level->GetBlockId(above) != Block::ByName("air")->GetId())
    {
        return;
    }

    int height = 1;
    while (level->GetBlockId(BlockPos(pos.x, pos.y - height, pos.z)) == GetId())
    {
        height++;
    }

    if (height >= 3)
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    int age       = GetPropertyValue(data, m_ageProperty);
    if (age == 15)
    {
        if (CanSurvive(level, above, nullptr))
        {
            level->SetBlock(above, this);
            level->RecordBlockChange(above);
        }
        else
        {
            DropAsItem(level, above, 0);
        }

        level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, 0));
    }
    else
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, age + 1));
    }

    level->RecordBlockChange(pos);
}
