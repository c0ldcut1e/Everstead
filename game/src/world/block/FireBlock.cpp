#include "FireBlock.h"

#include <algorithm>

#include "../../utils/Random.h"
#include "../Level.h"
#include "../chunk/Chunk.h"
#include "TntBlock.h"

FireBlock::FireBlock(const std::string &texturePath, const std::string &secondaryTexturePath) : Block(false, texturePath)
{
    SetSecondaryTexture(secondaryTexturePath);
    SetTransparent(true);
    SetCollidable(false);
    SetSelectable(false);
    SetReplaceable(true);
    SetFlatItem(true);
    SetRenderShape(eRenderShape_FIRE);
    SetLightEmission(15);
    SetLightColor(255, 190, 110);
    SetPlaceSound("fire.ignite", 1, 1.0f);
    m_ageProperty = AddProperty("age", kMaxAge + 1);
}

int FireBlock::GetEncouragement(Level *level, const BlockPos &pos) const
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return 0;
    }

    return Block::ById(level->GetBlockId(pos))->GetFireEncouragement();
}

bool FireBlock::IsBurnable(Level *level, const BlockPos &pos) const
{
    return GetEncouragement(level, pos) > 0;
}

int FireBlock::GetNeighborEncouragement(Level *level, const BlockPos &pos) const
{
    static const int offsets[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    int result                     = 0;
    for (int i = 0; i < 6; i++)
    {
        result = std::max(result, GetEncouragement(level, BlockPos(pos.x + offsets[i][0], pos.y + offsets[i][1], pos.z + offsets[i][2])));
    }

    return result;
}

bool FireBlock::HasBurnableNeighbor(Level *level, const BlockPos &pos) const
{
    return GetNeighborEncouragement(level, pos) > 0;
}

bool FireBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;
    if (pos.y <= 0 || pos.y >= Chunk::s_sizeY)
    {
        return false;
    }

    return Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)))->IsSolid() || HasBurnableNeighbor(level, pos);
}

bool FireBlock::IsRainingAround(Level *level, const BlockPos &pos) const
{
    return level->IsRainingAt(pos) || level->IsRainingAt(BlockPos(pos.x - 1, pos.y, pos.z)) ||
           level->IsRainingAt(BlockPos(pos.x + 1, pos.y, pos.z)) || level->IsRainingAt(BlockPos(pos.x, pos.y, pos.z - 1)) ||
           level->IsRainingAt(BlockPos(pos.x, pos.y, pos.z + 1));
}

void FireBlock::Extinguish(Level *level, const BlockPos &pos)
{
    static Block *air = Block::ByName("air");
    level->SetBlock(pos, air);
    level->RecordBlockChange(pos);
}

void FireBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (!CanSurvive(level, pos, nullptr))
    {
        Extinguish(level, pos);
        return;
    }

    level->ScheduleBlockForTick(pos, kTickDelay + level->GetRandom().NextInt(0, kTickDelayRange - 1), 0);
}

void FireBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (!CanSurvive(level, pos, nullptr))
    {
        Extinguish(level, pos);
    }
}

void FireBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!CanSurvive(level, pos, nullptr))
    {
        Extinguish(level, pos);
        return;
    }

    BlockPos below(pos.x, pos.y - 1, pos.z);
    bool infiniburn = Block::ById(level->GetBlockId(below)) == Block::ByName("brimstone");
    if (!infiniburn && IsRainingAround(level, pos))
    {
        Extinguish(level, pos);
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    int age       = GetPropertyValue(data, m_ageProperty);
    if (age < kMaxAge)
    {
        int newAge = std::min(age + level->GetRandom().NextInt(0, 2) / 2, kMaxAge);
        if (newAge != age)
        {
            level->SetBlockData(pos, WithPropertyValue(data, m_ageProperty, newAge));
            level->RecordBlockChange(pos);
        }
    }

    level->ScheduleBlockForTick(pos, kTickDelay + level->GetRandom().NextInt(0, kTickDelayRange - 1), 0);

    if (!infiniburn && !HasBurnableNeighbor(level, pos))
    {
        if (!Block::ById(level->GetBlockId(below))->IsSolid() || age > 3)
        {
            Extinguish(level, pos);
        }

        return;
    }

    if (!infiniburn && !IsBurnable(level, below) && age == kMaxAge && level->GetRandom().NextInt(0, 3) == 0)
    {
        Extinguish(level, pos);
        return;
    }

    if (!level->IsAreaLoaded(pos, 5))
    {
        return;
    }

    TryCatchFire(level, BlockPos(pos.x + 1, pos.y, pos.z), kSideBurnChance, age);
    TryCatchFire(level, BlockPos(pos.x - 1, pos.y, pos.z), kSideBurnChance, age);
    TryCatchFire(level, BlockPos(pos.x, pos.y - 1, pos.z), kVerticalBurnChance, age);
    TryCatchFire(level, BlockPos(pos.x, pos.y + 1, pos.z), kVerticalBurnChance, age);
    TryCatchFire(level, BlockPos(pos.x, pos.y, pos.z - 1), kSideBurnChance, age);
    TryCatchFire(level, BlockPos(pos.x, pos.y, pos.z + 1), kSideBurnChance, age);

    for (int x = pos.x - 1; x <= pos.x + 1; x++)
    {
        for (int z = pos.z - 1; z <= pos.z + 1; z++)
        {
            for (int y = pos.y - 1; y <= pos.y + 4; y++)
            {
                if ((x == pos.x && y == pos.y && z == pos.z) || y <= 0 || y >= Chunk::s_sizeY)
                {
                    continue;
                }

                BlockPos target(x, y, z);
                if (level->GetBlockId(target) != 0)
                {
                    continue;
                }

                int odds = 100;
                if (y > pos.y + 1)
                {
                    odds += (y - (pos.y + 1)) * 100;
                }

                int encouragement = GetNeighborEncouragement(level, target);
                if (encouragement <= 0)
                {
                    continue;
                }

                int chance = (encouragement + 40 + 14) / (age + 30);
                if (chance > 0 && level->GetRandom().NextInt(0, odds - 1) <= chance && !IsRainingAround(level, target))
                {
                    int spreadAge = std::min(age + level->GetRandom().NextInt(0, 4) / 4, kMaxAge);
                    level->SetBlock(target, this, nullptr, WithPropertyValue(0, m_ageProperty, spreadAge));
                    level->RecordBlockChange(target);
                }
            }
        }
    }
}

void FireBlock::TryCatchFire(Level *level, const BlockPos &pos, int chance, int age)
{
    if (pos.y <= 0 || pos.y >= Chunk::s_sizeY)
    {
        return;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    if (level->GetRandom().NextInt(0, chance - 1) >= block->GetFlammability())
    {
        return;
    }

    if (level->GetRandom().NextInt(0, age + 9) < 5 && !level->IsRainingAt(pos))
    {
        int newAge = std::min(age + level->GetRandom().NextInt(0, 4) / 4, kMaxAge);
        level->SetBlock(pos, this, nullptr, WithPropertyValue(0, m_ageProperty, newAge));
        level->RecordBlockChange(pos);
    }
    else
    {
        Extinguish(level, pos);
    }

    if (block == Block::ByName("tnt"))
    {
        TntBlock::Ignite(level, pos);
    }
}
