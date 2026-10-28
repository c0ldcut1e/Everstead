#include "StemBlock.h"

#include <algorithm>

#include "../Level.h"
#include "CropBlock.h"

StemBlock::StemBlock(const std::string &texturePrefix, const std::string &fruitName, const std::string &attachedName)
    : Block(false, texturePrefix + "0.png")
{
    m_ageProperty  = AddProperty("age", kMaxAge + 1);
    m_fruitName    = fruitName;
    m_attachedName = attachedName;
    for (int i = 0; i <= kMaxAge; i++)
    {
        AddTextureVariant(texturePrefix + std::to_string(i) + ".png");
    }

    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetFlatItem(true);
    SetRandomTicking(true);
    SetNeedsSupportBelow(true);
    SetBreakSound("dig.wood");
}

void StemBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || level->GetRawBrightness(BlockPos(pos.x, pos.y + 1, pos.z)) < 9 ||
        level->GetRandom().NextInt(0, (int) (25.0f / CropBlock::GetGrowthSpeed(level, pos, this))) != 0)
    {
        return;
    }

    int age = GetPropertyValue(level->GetBlockData(pos), m_ageProperty);
    if (age < kMaxAge)
    {
        level->SetBlockData(pos, WithAge(age + 1));
        level->RecordBlockChange(pos);
        return;
    }

    GrowFruit(level, pos);
}

void StemBlock::GrowFruit(Level *level, const BlockPos &pos) const
{
    Direction *directions[] = {Direction::north, Direction::south, Direction::west, Direction::east};
    Block *fruit            = Block::ByName(m_fruitName);
    for (int i = 0; i < 4; i++)
    {
        if (Block::ById(level->GetBlockId(BlockPos(pos.x + directions[i]->dx, pos.y, pos.z + directions[i]->dz))) == fruit)
        {
            return;
        }
    }

    Direction *direction = directions[level->GetRandom().NextInt(0, 3)];
    BlockPos target(pos.x + direction->dx, pos.y, pos.z + direction->dz);
    Block *below = Block::ById(level->GetBlockId(BlockPos(target.x, target.y - 1, target.z)));
    if (Block::ById(level->GetBlockId(target)) != Block::ByName("air") ||
        (below != Block::ByName("farmland") && below != Block::ByName("dirt") && below != Block::ByName("grass_block")))
    {
        return;
    }

    level->SetBlock(target, fruit);
    level->RecordBlockChange(target);
    level->SetBlock(pos, Block::ByName(m_attachedName), nullptr, Block::ByName(m_attachedName)->GetPlacementData(direction));
    level->RecordBlockChange(pos);
}

bool StemBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    return level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)) != 0 && (level->GetLightLevel(pos) >= 8 || level->GetSkyLightLevel(pos) == 15);
}

bool StemBlock::CanPlaceAt(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) == Block::ByName("farmland");
}

Block *StemBlock::GetFruit() const
{
    return Block::ByName(m_fruitName);
}

Block *StemBlock::GetAttachedStem() const
{
    return Block::ByName(m_attachedName);
}

bool StemBlock::CanBoneMeal(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty) < kMaxAge;
}

void StemBlock::BoneMeal(Level *level, const BlockPos &pos)
{
    int age = std::min(GetPropertyValue(level->GetBlockData(pos), m_ageProperty) + level->GetRandom().NextInt(2, 5), kMaxAge);
    level->SetBlockData(pos, WithAge(age));
    level->RecordBlockChange(pos);
    if (age == kMaxAge)
    {
        RandomTick(level, pos);
    }
}

uint16_t StemBlock::WithAge(int age) const
{
    return WithPropertyValue(0, m_ageProperty, age);
}

Block::UVRect StemBlock::GetCrossAtlasUVRect(uint16_t data) const
{
    return GetTextureVariantAtlasUVRect(GetPropertyValue(data, m_ageProperty));
}

glm::dvec3 StemBlock::GetCrossOffset(const BlockPos &pos, uint16_t data) const
{
    (void) pos;

    return glm::dvec3(0.0, ((GetPropertyValue(data, m_ageProperty) + 1) * 2.0 - 1.0) / 16.0 - 1.0, 0.0);
}

AABB StemBlock::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    double height = (GetPropertyValue(data, m_ageProperty) + 1) * 2.0 / 16.0;
    return AABB(glm::dvec3(pos.x + 7.0 / 16.0, pos.y, pos.z + 7.0 / 16.0), glm::dvec3(pos.x + 9.0 / 16.0, pos.y + height, pos.z + 9.0 / 16.0));
}
