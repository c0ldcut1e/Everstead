#include "FlintAndSteelDispenseBehavior.h"

#include "../../utils/Direction.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../block/TntBlock.h"
#include "../blockentity/DispenserBlockEntity.h"

bool FlintAndSteelDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    Level *level        = dispenser->GetLevel();
    const BlockPos &pos = dispenser->GetPos();
    BlockPos front(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    Block *block = Block::ById(level->GetBlockId(front));
    if (block == Block::ByName("air"))
    {
        level->SetBlock(front, Block::ByName("fire"));
        level->RecordBlockChange(front);
        item->ConsumeOne();
        return true;
    }

    if (block == Block::ByName("tnt"))
    {
        TntBlock::Ignite(level, front);
        level->SetBlock(front, Block::ByName("air"));
        level->RecordBlockChange(front);
        return true;
    }

    return false;
}
