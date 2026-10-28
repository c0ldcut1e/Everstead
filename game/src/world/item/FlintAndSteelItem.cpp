#include "FlintAndSteelItem.h"

#include "../../entity/Inventory.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../block/TntBlock.h"
#include "../chunk/Chunk.h"
#include "ItemInstance.h"

FlintAndSteelItem::FlintAndSteelItem()
{}

bool FlintAndSteelItem::CanUse(Level *level, const BlockPos &pos, Direction *face) const
{
    if (Block::ById(level->GetBlockId(pos)) == Block::ByName("tnt"))
    {
        return true;
    }

    BlockPos target(pos.x + face->dx, pos.y + face->dy, pos.z + face->dz);
    if (target.y < 0 || target.y >= Chunk::s_sizeY || !level->GetWorldBorder().IsWithinBounds(target))
    {
        return false;
    }

    Block *fire = Block::ByName("fire");
    return level->GetBlockId(target) == 0 && fire->CanSurvive(level, target, face);
}

void FlintAndSteelItem::Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const
{
    if (!CanUse(level, pos, face))
    {
        return;
    }

    if (Block::ById(level->GetBlockId(pos)) == Block::ByName("tnt"))
    {
        TntBlock::Ignite(level, pos);
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
    }
    else
    {
        BlockPos target(pos.x + face->dx, pos.y + face->dy, pos.z + face->dz);
        level->PlaySound("fire.ignite1", glm::dvec3((double) target.x + 0.5, (double) target.y + 0.5, (double) target.z + 0.5), 1.0f,
                         level->GetRandom().NextFloat() * 0.4f + 0.8f);
        level->SetBlock(target, Block::ByName("fire"), face);
        level->RecordBlockChange(target);
    }

    if (!GameType::HasInfiniteItems(player->GetGameType()))
    {
        player->GetInventory()->GetSelectedItem()->HurtAndBreak(1, player);
    }
}
