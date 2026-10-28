#include "BoneMealItem.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../block/CropBlock.h"
#include "../block/SaplingBlock.h"
#include "../block/StemBlock.h"
#include "../block/SweetBerryBushBlock.h"
#include "../chunk/Chunk.h"

BoneMealItem::BoneMealItem()
{}

bool BoneMealItem::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    (void) face;
    Block *block = Block::ById(level->GetBlockId(pos));
    if (dynamic_cast<SaplingBlock *>(block))
    {
        return true;
    }

    CropBlock *crop = dynamic_cast<CropBlock *>(block);
    StemBlock *stem = dynamic_cast<StemBlock *>(block);
    if (crop || stem)
    {
        uint16_t data = level->GetBlockData(pos);
        return crop ? crop->CanBoneMeal(data) : stem->CanBoneMeal(data);
    }

    SweetBerryBushBlock *bush = dynamic_cast<SweetBerryBushBlock *>(block);
    if (bush)
    {
        return bush->CanBoneMeal(level->GetBlockData(pos));
    }

    bool airAbove = pos.y + 1 < Chunk::s_sizeY && Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z))) == Block::ByName("air");
    return airAbove && (block == Block::ByName("grass_block") || block == Block::ByName("grass") || block == Block::ByName("fern"));
}

void BoneMealItem::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    (void) face;
    (void) look;

    Block *block          = Block::ById(level->GetBlockId(pos));
    SaplingBlock *sapling = dynamic_cast<SaplingBlock *>(block);
    if (sapling)
    {
        if (level->GetRandom().NextInt(0, 99) < 45)
        {
            sapling->Grow(level, pos);
        }

        return;
    }

    CropBlock *crop = dynamic_cast<CropBlock *>(block);
    if (crop)
    {
        crop->BoneMeal(level, pos);
        return;
    }

    StemBlock *stem = dynamic_cast<StemBlock *>(block);
    if (stem)
    {
        stem->BoneMeal(level, pos);
        return;
    }

    SweetBerryBushBlock *bush = dynamic_cast<SweetBerryBushBlock *>(block);
    if (bush)
    {
        bush->BoneMeal(level, pos);
        return;
    }

    if (block == Block::ByName("grass") || block == Block::ByName("fern"))
    {
        level->SetBlock(pos, Block::ByName(block == Block::ByName("grass") ? "tall_grass" : "large_fern"));
        level->RecordBlockChange(pos);
        return;
    }

    const char *flowerNames[] = {"dandelion", "poppy",        "blue_orchid", "allium",     "azure_bluet",
                                 "red_tulip", "orange_tulip", "white_tulip", "pink_tulip", "oxeye_daisy"};
    const int flowerCount     = (int) (sizeof(flowerNames) / sizeof(flowerNames[0]));
    Block *air                = Block::ByName("air");
    Block *grassBlock         = Block::ByName("grass_block");
    for (int i = 0; i < 64; i++)
    {
        BlockPos target(pos.x + level->GetRandom().NextInt(-3, 3), pos.y + level->GetRandom().NextInt(-1, 1) + 1,
                        pos.z + level->GetRandom().NextInt(-3, 3));
        if (target.y < 1 || target.y >= Chunk::s_sizeY || Block::ById(level->GetBlockId(target)) != air ||
            Block::ById(level->GetBlockId(BlockPos(target.x, target.y - 1, target.z))) != grassBlock)
        {
            continue;
        }

        Block *plant = level->GetRandom().NextInt(0, 7) == 0 ? Block::ByName(flowerNames[level->GetRandom().NextInt(0, flowerCount - 1)])
                                                             : Block::ByName("grass");
        level->SetBlock(target, plant);
        level->RecordBlockChange(target);
    }
}

void BoneMealItem::SpawnUseEffects(Level *level, const BlockPos &pos) const
{
    Block *block = Block::ById(level->GetBlockId(pos));
    if (!block || block == Block::ByName("air"))
    {
        return;
    }

    bool onSapling = dynamic_cast<SaplingBlock *>(block) || dynamic_cast<CropBlock *>(block) || dynamic_cast<StemBlock *>(block) ||
                     dynamic_cast<SweetBerryBushBlock *>(block) || block == Block::ByName("grass") || block == Block::ByName("fern") ||
                     block == Block::ByName("tall_grass") || block == Block::ByName("large_fern");
    double height  = onSapling ? 0.8 : 1.0;
    double baseY   = onSapling ? pos.y : pos.y + 1.0;
    for (int i = 0; i < 15; i++)
    {
        ParticleSpawnParams params;
        params.type        = eParticleType_HappyVillager;
        params.position    = glm::dvec3(pos.x + level->GetRandom().NextDouble(), baseY + level->GetRandom().NextDouble() * height,
                                        pos.z + level->GetRandom().NextDouble());
        params.velocity    = glm::dvec3(level->GetRandom().NextGaussian() * 0.02, level->GetRandom().NextGaussian() * 0.02,
                                        level->GetRandom().NextGaussian() * 0.02);
        params.hasVelocity = true;
        level->SpawnParticle(params);
    }
}
