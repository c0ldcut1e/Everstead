#include "SweetBerryBushBlock.h"

#include <algorithm>

#include "../../utils/Random.h"
#include "../Level.h"
#include "../item/Item.h"

SweetBerryBushBlock::SweetBerryBushBlock() : Block(false, "assets/texture/block/sweet_berry_bush_stage0.png")
{
    m_ageProperty = AddProperty("age", kMaxAge + 1);
    for (int i = 0; i <= kMaxAge; i++)
    {
        AddTextureVariant("assets/texture/block/sweet_berry_bush_stage" + std::to_string(i) + ".png");
    }

    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetRandomTicking(true);
    SetNeedsSupportBelow(true);
    SetBreakSound("sweet_berry_bush.break", 2);
    SetPlaceSound("sweet_berry_bush.place", 5, 1.0f);
    SetAABB(AABB(glm::dvec3(1.0 / 16.0, 0.0, 1.0 / 16.0), glm::dvec3(15.0 / 16.0, 1.0, 15.0 / 16.0)));
}

void SweetBerryBushBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    int age       = GetPropertyValue(data, m_ageProperty);
    if (age < kMaxAge && level->GetRandom().NextInt(0, 4) == 0 && level->GetRawBrightness(BlockPos(pos.x, pos.y + 1, pos.z)) >= 9)
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, age + 1));
        level->RecordBlockChange(pos);
    }
}

bool SweetBerryBushBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == Block::ByName("grass_block") || below == Block::ByName("dirt") || below == Block::ByName("coarse_dirt") ||
           below == Block::ByName("podzol") || below == Block::ByName("farmland");
}

bool SweetBerryBushBlock::IsUsable(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty) > 1;
}

bool SweetBerryBushBlock::OnUse(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    int age       = GetPropertyValue(data, m_ageProperty);
    if (age <= 1)
    {
        return false;
    }

    if (!level->IsClientSide())
    {
        int count = 1 + level->GetRandom().NextInt(0, 1) + (age == kMaxAge ? 1 : 0);
        for (int i = 0; i < count; i++)
        {
            SpawnDrop(level, pos, Item::ByName("sweet_berries"), 0);
        }
    }

    level->PlaySound("sweet_berries.pick_from_bush" + std::to_string(level->GetRandom().NextInt(1, 2)),
                     glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f, 0.8f + level->GetRandom().NextFloat() * 0.4f);
    level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, 1));
    level->RecordBlockChange(pos);
    return true;
}

Block::UVRect SweetBerryBushBlock::GetCrossAtlasUVRect(uint16_t data) const
{
    return GetTextureVariantAtlasUVRect(GetCrossTextureVariant(data));
}

int SweetBerryBushBlock::GetCrossTextureVariant(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty);
}

AABB SweetBerryBushBlock::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    if (GetPropertyValue(data, m_ageProperty) == 0)
    {
        return AABB(glm::dvec3(pos.x + 3.0 / 16.0, pos.y, pos.z + 3.0 / 16.0), glm::dvec3(pos.x + 13.0 / 16.0, pos.y + 0.5, pos.z + 13.0 / 16.0));
    }

    return AABB(glm::dvec3(pos.x + 1.0 / 16.0, pos.y, pos.z + 1.0 / 16.0), glm::dvec3(pos.x + 15.0 / 16.0, pos.y + 1.0, pos.z + 15.0 / 16.0));
}

uint16_t SweetBerryBushBlock::GetAgeData(int age) const
{
    return WithPropertyValue(0, m_ageProperty, age);
}

bool SweetBerryBushBlock::CanBoneMeal(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty) < kMaxAge;
}

void SweetBerryBushBlock::BoneMeal(Level *level, const BlockPos &pos) const
{
    uint16_t data = level->GetBlockData(pos);
    level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, std::min(GetPropertyValue(data, m_ageProperty) + 1, kMaxAge)));
    level->RecordBlockChange(pos);
}
