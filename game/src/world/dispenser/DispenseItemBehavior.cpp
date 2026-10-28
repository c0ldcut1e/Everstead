#include "DispenseItemBehavior.h"

#include <memory>
#include <utility>

#include "../../entity/item/ItemEntity.h"
#include "../../network/packet/s2c/ParticleS2CPacket.h"
#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../ServerLevel.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/BoatItem.h"
#include "../item/BucketItem.h"
#include "../item/Item.h"
#include "../item/SpawnEggItem.h"
#include "../item/ThrowableItem.h"
#include "ArmorDispenseBehavior.h"
#include "ArrowDispenseBehavior.h"
#include "BoatDispenseBehavior.h"
#include "BoneMealDispenseBehavior.h"
#include "BucketDispenseBehavior.h"
#include "EmptyBucketDispenseBehavior.h"
#include "FlintAndSteelDispenseBehavior.h"
#include "ShearsDispenseBehavior.h"
#include "SpawnEggDispenseBehavior.h"
#include "ThrowableDispenseBehavior.h"
#include "TntDispenseBehavior.h"

ItemInstance DispenseItemBehavior::Dispense(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance item)
{
    Level *level        = dispenser->GetLevel();
    const BlockPos &pos = dispenser->GetPos();
    PlaySound(level, pos, Execute(dispenser, facing, &item));

    Random &random = level->GetRandom();
    glm::dvec3 center(pos.x + facing->dx * 0.6 + 0.5, pos.y + facing->dy * 0.6 + 0.5, pos.z + facing->dz * 0.6 + 0.5);
    for (int i = 0; i < 10; i++)
    {
        double speed = random.NextDouble() * 0.2 + 0.01;
        glm::dvec3 position(center.x + facing->dx * 0.01 + (random.NextDouble() - 0.5) * facing->dz * 0.5,
                            center.y + facing->dy * 0.01 + (random.NextDouble() - 0.5) * facing->dy * 0.5,
                            center.z + facing->dz * 0.01 + (random.NextDouble() - 0.5) * facing->dx * 0.5);
        glm::dvec3 velocity(facing->dx * speed + random.NextGaussian() * 0.01, facing->dy * speed + random.NextGaussian() * 0.01,
                            facing->dz * speed + random.NextGaussian() * 0.01);
        ((ServerLevel *) level)
                ->SendToNearby(position, 64.0, std::shared_ptr<Packet>(new ParticleS2CPacket(eParticleType_Smoke, position, velocity, 1.0f, 0)));
    }

    return item;
}

DispenseItemBehavior *DispenseItemBehavior::Get(Item *item)
{
    static ArrowDispenseBehavior s_arrow;
    static ThrowableDispenseBehavior s_throwable;
    static BucketDispenseBehavior s_bucket;
    static EmptyBucketDispenseBehavior s_emptyBucket;
    static FlintAndSteelDispenseBehavior s_flintAndSteel;
    static BoneMealDispenseBehavior s_boneMeal;
    static TntDispenseBehavior s_tnt;
    static ShearsDispenseBehavior s_shears;
    static ArmorDispenseBehavior s_armor;
    static BoatDispenseBehavior s_boat;
    static SpawnEggDispenseBehavior s_spawnEgg;

    if (item == Item::ByName("arrow"))
    {
        return &s_arrow;
    }

    if (dynamic_cast<ThrowableItem *>(item))
    {
        return &s_throwable;
    }

    BucketItem *bucket = dynamic_cast<BucketItem *>(item);
    if (bucket)
    {
        return bucket->GetLiquid() ? (DispenseItemBehavior *) &s_bucket : &s_emptyBucket;
    }

    if (item == Item::ByName("flint_and_steel"))
    {
        return &s_flintAndSteel;
    }

    if (item == Item::ByName("bone_meal"))
    {
        return &s_boneMeal;
    }

    if (item == Item::ByName("tnt"))
    {
        return &s_tnt;
    }

    if (item == Item::ByName("shears"))
    {
        return &s_shears;
    }

    if (item->GetArmorSlot() >= 0)
    {
        return &s_armor;
    }

    if (dynamic_cast<BoatItem *>(item))
    {
        return &s_boat;
    }

    if (dynamic_cast<SpawnEggItem *>(item))
    {
        return &s_spawnEgg;
    }

    return GetDefault();
}

DispenseItemBehavior *DispenseItemBehavior::GetDefault()
{
    static DispenseItemBehavior s_default;
    return &s_default;
}

bool DispenseItemBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    ItemInstance single(*item);
    single.SetCount(1);
    Drop(dispenser->GetLevel(), dispenser->GetPos(), facing, single);
    item->SetCount(item->GetCount() - 1);
    return true;
}

void DispenseItemBehavior::PlaySound(Level *level, const BlockPos &pos, bool success) const
{
    level->PlaySound("random.click", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f, success ? 1.0f : 1.2f);
}

glm::dvec3 DispenseItemBehavior::GetDispensePosition(const BlockPos &pos, Direction *facing)
{
    return glm::dvec3(pos.x + 0.5 + facing->dx * 0.7, pos.y + 0.5 + facing->dy * 0.7, pos.z + 0.5 + facing->dz * 0.7);
}

void DispenseItemBehavior::Drop(Level *level, const BlockPos &pos, Direction *facing, const ItemInstance &item)
{
    Random &random      = level->GetRandom();
    glm::dvec3 position = GetDispensePosition(pos, facing);
    position.y -= facing->dy != 0 ? 0.125 : 0.15625;
    double speed = random.NextDouble() * 0.1 + 0.2;
    glm::dvec3 velocity(facing->dx * speed + random.NextGaussian() * 0.0075 * 6.0, 0.2 + random.NextGaussian() * 0.0075 * 6.0,
                        facing->dz * speed + random.NextGaussian() * 0.0075 * 6.0);
    std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(level));
    itemEntity->Sync(position, velocity, item, 0, 0);
    level->AddEntity(std::move(itemEntity));
}
