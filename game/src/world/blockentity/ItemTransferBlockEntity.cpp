#include "ItemTransferBlockEntity.h"

#include <algorithm>

#include "../../entity/item/Minecart.h"
#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../utils/Direction.h"
#include "../ServerLevel.h"
#include "../block/ChestBlock.h"
#include "../crafting/FurnaceRecipes.h"
#include "../item/Item.h"
#include "../save/WorldSave.h"
#include "ChestBlockEntity.h"
#include "FurnaceBlockEntity.h"

ItemTransferBlockEntity::ItemTransferBlockEntity(Level *level, const BlockPos &pos, int rows, int columns)
    : BlockEntity(level, pos), Container(rows, columns)
{
    m_tickedGameTime = 0;
}

AbstractContainer *ItemTransferBlockEntity::GetContainerAt(Level *level, const BlockPos &pos, BlockPos *partnerPos,
                                                           std::unique_ptr<CompoundContainer> *compoundChest)
{
    *partnerPos              = pos;
    BlockEntity *blockEntity = level->GetBlockEntity(pos);
    ChestBlockEntity *chest  = dynamic_cast<ChestBlockEntity *>(blockEntity);
    if (!chest)
    {
        AbstractContainer *container = dynamic_cast<AbstractContainer *>(blockEntity);
        if (container)
        {
            return container;
        }

        AABB box(glm::dvec3(pos.x, pos.y, pos.z), glm::dvec3(pos.x + 1.0, pos.y + 1.0, pos.z + 1.0));
        const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
        for (size_t i = 0; i < entities.size(); i++)
        {
            Minecart *minecart = dynamic_cast<Minecart *>(entities[i].get());
            if (minecart && minecart->GetContainer() && minecart->GetAABB().Intersects(box))
            {
                return minecart->GetContainer();
            }
        }

        return nullptr;
    }

    Direction *partner = ((ChestBlock *) Block::ByName("chest"))->GetPartner(level->GetBlockData(pos));
    if (!partner)
    {
        return chest;
    }

    *partnerPos                    = BlockPos(pos.x + partner->dx, pos.y, pos.z + partner->dz);
    ChestBlockEntity *partnerChest = dynamic_cast<ChestBlockEntity *>(level->GetBlockEntity(*partnerPos));
    if (!partnerChest)
    {
        return chest;
    }

    if (partner == Direction::west || partner == Direction::north)
    {
        compoundChest->reset(new CompoundContainer(partnerChest, chest));
    }
    else
    {
        compoundChest->reset(new CompoundContainer(chest, partnerChest));
    }

    return compoundChest->get();
}

// Furnaces take input from the top and fuel from the sides, matching their menu slots
bool ItemTransferBlockEntity::CanInsert(AbstractContainer *container, int slot, const ItemInstance &item, Direction *side)
{
    if (!dynamic_cast<FurnaceBlockEntity *>(container))
    {
        return true;
    }

    if (side == Direction::up)
    {
        return slot == FurnaceContainer::s_inputSlot;
    }

    if (side == Direction::down || slot != FurnaceContainer::s_fuelSlot)
    {
        return false;
    }

    Item *bucket = Item::ByName("bucket");
    return FurnaceRecipes::GetInstance()->GetBurnDuration(item) > 0 ||
           (item.GetItem() == bucket && container->GetItem(FurnaceContainer::s_fuelSlot)->GetItem() != bucket);
}

ItemInstance ItemTransferBlockEntity::Insert(AbstractContainer *container, ItemInstance item, Direction *side, ItemTransferBlockEntity *source)
{
    bool wasEmpty = container->IsEmpty();
    for (int i = 0; i < container->GetSize() && !item.IsEmpty(); i++)
    {
        if (!CanInsert(container, i, item, side))
        {
            continue;
        }

        ItemInstance *slotItem = container->GetItem(i);
        if (slotItem->IsEmpty())
        {
            container->SetItem(i, item);
            item = ItemInstance();
        }
        else if (slotItem->IsSameItem(item) && slotItem->GetCount() < slotItem->GetMaxStackSize())
        {
            int moved = std::min(item.GetCount(), slotItem->GetMaxStackSize() - slotItem->GetCount());
            slotItem->SetCount(slotItem->GetCount() + moved);
            item.SetCount(item.GetCount() - moved);
        }
    }

    ItemTransferBlockEntity *target = dynamic_cast<ItemTransferBlockEntity *>(container);
    if (wasEmpty && target && target != source && !container->IsEmpty())
    {
        target->OnFirstItemReceived(source ? source->m_tickedGameTime : -1);
    }

    return item;
}

void ItemTransferBlockEntity::MarkDirty(Level *level, const BlockPos &pos, const BlockPos &partnerPos)
{
    ((ServerLevel *) level)->MarkBlockChunkDirty(pos);
    ((ServerLevel *) level)->MarkBlockChunkDirty(partnerPos);
}

void ItemTransferBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);

    std::unique_ptr<ListTag> items(new ListTag());
    for (int i = 0; i < GetSize(); i++)
    {
        const ItemInstance *item = GetItem(i);
        if (item->IsEmpty())
        {
            continue;
        }

        std::unique_ptr<CompoundTag> itemTag = WorldSave::CreateItemTag(*item);
        itemTag->PutByte("Slot", (int8_t) i);
        items->Add(std::move(itemTag));
    }

    tag->Put("Items", std::move(items));
}

void ItemTransferBlockEntity::Load(const CompoundTag *tag)
{
    ListTag *items = tag->GetList("Items");
    for (size_t i = 0; items && i < items->Size(); i++)
    {
        const CompoundTag *itemTag = (const CompoundTag *) items->Get(i);
        int slot                   = itemTag->GetByte("Slot");
        if (slot >= 0 && slot < GetSize())
        {
            SetItem(slot, WorldSave::ReadItemTag(itemTag));
        }
    }
}

void ItemTransferBlockEntity::OnFirstItemReceived(int64_t sourceTickedGameTime)
{
    (void) sourceTickedGameTime;
}
