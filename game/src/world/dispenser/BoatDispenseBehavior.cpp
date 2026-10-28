#include "BoatDispenseBehavior.h"

#include <cmath>
#include <memory>
#include <utility>

#include "../../entity/item/Boat.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/BoatItem.h"

#include <glm/trigonometric.hpp>

bool BoatDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    Level *level        = dispenser->GetLevel();
    const BlockPos &pos = dispenser->GetPos();
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    glm::dvec3 position(pos.x + 0.5 + facing->dx * 1.125, pos.y + 0.5 + facing->dy * 1.125, pos.z + 0.5 + facing->dz * 1.125);
    Block *block = Block::ById(level->GetBlockId(front));
    Block *below = Block::ById(level->GetBlockId(BlockPos(front.x, front.y - 1, front.z)));
    if (block->IsLiquid() && !block->IsLava())
    {
        position.y += 1.0;
    }
    else if (block != Block::ByName("air") || !below->IsLiquid() || below->IsLava())
    {
        return DispenseItemBehavior::Execute(dispenser, facing, item);
    }

    std::unique_ptr<Boat> boat(new Boat(level));
    boat->SetWoodType(((BoatItem *) item->GetItem())->GetWoodType());
    boat->SetPosition(position);
    boat->SetRotation(glm::degrees((float) atan2(facing->dz, facing->dx)), 0.0f);
    level->AddEntity(std::move(boat));
    item->SetCount(item->GetCount() - 1);
    return true;
}
