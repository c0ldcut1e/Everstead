#include "ShearsItem.h"

#include <cmath>
#include <memory>

#include "../../entity/item/ItemEntity.h"
#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../block/LeavesBlock.h"
#include "ItemInstance.h"

ShearsItem::ShearsItem()
{}

float ShearsItem::GetDestroySpeed(Block *block) const
{
    if (LeavesBlock::IsLeaves(block))
    {
        return 15.0f;
    }

    if (block->GetResourceLocation().GetPath().ends_with("_wool"))
    {
        return 5.0f;
    }

    return 1.0f;
}

int ShearsItem::GetMineDamage(Block *block) const
{
    return LeavesBlock::IsLeaves(block) ? 1 : 0;
}

bool ShearsItem::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    (void) face;
    return Block::ById(level->GetBlockId(pos)) == Block::ByName("pumpkin");
}

void ShearsItem::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    Random &random = level->GetRandom();

    Direction *front = face;
    if (face == Direction::up || face == Direction::down)
    {
        if (fabs(look.x) > fabs(look.z))
        {
            front = look.x > 0.0 ? Direction::west : Direction::east;
        }
        else
        {
            front = look.z > 0.0 ? Direction::north : Direction::south;
        }
    }

    Block *carvedPumpkin = Block::ByName("carved_pumpkin");
    level->SetBlock(pos, carvedPumpkin);
    level->SetBlockData(pos, carvedPumpkin->GetPlacementState(level, pos, front, glm::dvec3(), glm::dvec3(-front->dx, 0.0, -front->dz)));
    level->RecordBlockChange(pos);

    if (level->IsClientSide())
    {
        return;
    }

    glm::dvec3 position(pos.x + 0.5 + front->dx * 0.65, pos.y + 0.1, pos.z + 0.5 + front->dz * 0.65);
    glm::dvec3 velocity(front->dx * 0.05 + random.NextDouble() * 0.02, 0.05, front->dz * 0.05 + random.NextDouble() * 0.02);
    std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(level));
    itemEntity->Sync(position, velocity, ItemInstance(Item::ByName("pumpkin_seeds"), 4, 0), 0, 10);
    level->AddEntity(std::move(itemEntity));
}
