#include "BucketItem.h"

#include "../../entity/Inventory.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"

BucketItem::BucketItem(Block *liquid)
{
    m_liquid = liquid;
}

Block *BucketItem::GetLiquid() const
{
    return m_liquid;
}

Block *BucketItem::GetLightBlock() const
{
    return m_liquid;
}

bool BucketItem::CanUse(Level *level, const BlockPos &pos, Direction *face) const
{
    if (!m_liquid)
    {
        return Block::ById(level->GetBlockId(pos))->IsLiquid() && level->GetBlockData(pos) == 0;
    }

    BlockPos target(pos.x + face->dx, pos.y + face->dy, pos.z + face->dz);
    if (target.y < 0 || target.y >= Chunk::s_sizeY || !level->GetWorldBorder().IsWithinBounds(target))
    {
        return false;
    }

    Block *block = Block::ById(level->GetBlockId(target));
    return !block->IsSolid() || block->IsLiquid() || block == Block::ByName("air");
}

// This has to run before the liquid is picked up
ItemInstance BucketItem::UpdateHeldItem(Player *player, Level *level, const BlockPos &pos) const
{
    Inventory *inventory   = player->GetInventory();
    ItemInstance *heldItem = inventory->GetSelectedItem();
    bool infiniteItems     = GameType::HasInfiniteItems(player->GetGameType());
    if (m_liquid)
    {
        if (!infiniteItems)
        {
            *heldItem = ItemInstance(Item::ByName("bucket"), 1);
        }

        return ItemInstance();
    }

    ItemInstance filled(Item::ByName(Block::ById(level->GetBlockId(pos))->IsLava() ? "lava_bucket" : "water_bucket"), 1);
    if (infiniteItems)
    {
        for (int i = 0; i < Inventory::s_totalSize; i++)
        {
            if (inventory->GetSlot(i)->IsSameItem(filled))
            {
                return ItemInstance();
            }
        }

        inventory->AddItem(filled);
        return ItemInstance();
    }

    if (heldItem->GetCount() == 1)
    {
        *heldItem = filled;
        return ItemInstance();
    }

    heldItem->ConsumeOne();
    inventory->AddItem(filled);
    return filled;
}

void BucketItem::Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const
{
    if (!CanUse(level, pos, face))
    {
        return;
    }

    player->DropItem(UpdateHeldItem(player, level, pos));
    if (!m_liquid)
    {
        level->SetBlock(pos, Block::ByName("air"));
        return;
    }

    PlaceLiquid(level, BlockPos(pos.x + face->dx, pos.y + face->dy, pos.z + face->dz));
}

void BucketItem::PlaceLiquid(Level *level, const BlockPos &target) const
{
    Block *block = Block::ById(level->GetBlockId(target));
    if (!block->IsLiquid() && block != Block::ByName("air"))
    {
        block->DropAsItem(level, target, level->GetBlockData(target));
    }

    level->SetBlock(target, m_liquid);
}
