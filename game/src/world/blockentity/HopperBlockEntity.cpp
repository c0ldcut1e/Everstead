#include "HopperBlockEntity.h"

#include <vector>

#include "../../entity/item/ItemEntity.h"
#include "../../tag/CompoundTag.h"
#include "../../utils/Direction.h"
#include "../ServerLevel.h"
#include "../block/HopperBlock.h"
#include "../item/Item.h"
#include "FurnaceBlockEntity.h"

HopperBlockEntity::HopperBlockEntity(Level *level, const BlockPos &pos) : ItemTransferBlockEntity(level, pos, 1, s_slotCount)
{
    m_cooldown = -1;
}

const char *HopperBlockEntity::GetId() const
{
    return "hopper";
}

void HopperBlockEntity::Tick()
{
    if (m_level->IsClientSide())
    {
        return;
    }

    m_cooldown--;
    m_tickedGameTime = m_level->GetGameTime();
    if (m_cooldown > 0)
    {
        return;
    }

    m_cooldown         = 0;
    uint16_t data      = m_level->GetBlockData(m_pos);
    HopperBlock *block = (HopperBlock *) Block::ById(m_level->GetBlockId(m_pos));
    if (!block->IsEnabled(data))
    {
        return;
    }

    bool moved = !IsEmpty() && PushItems(block->GetFacing(data));
    moved      = SuckInItems(m_level, this, BlockPos(m_pos.x, m_pos.y + 1, m_pos.z),
                             AABB(glm::dvec3(m_pos.x, m_pos.y + 0.5, m_pos.z), glm::dvec3(m_pos.x + 1.0, m_pos.y + 2.0, m_pos.z + 1.0)), this,
                             &m_compoundChest) ||
                 moved;
    if (moved)
    {
        m_cooldown = 8;
        ((ServerLevel *) m_level)->MarkBlockChunkDirty(m_pos);
    }
}

bool HopperBlockEntity::PushItems(Direction *facing)
{
    BlockPos targetPos(m_pos.x + facing->dx, m_pos.y + facing->dy, m_pos.z + facing->dz);
    BlockPos partnerPos;
    AbstractContainer *target = GetContainerAt(m_level, targetPos, &partnerPos, &m_compoundChest);
    if (!target)
    {
        return false;
    }

    for (int i = 0; i < GetSize(); i++)
    {
        ItemInstance *item = GetItem(i);
        if (item->IsEmpty())
        {
            continue;
        }

        ItemInstance single(*item);
        single.SetCount(1);
        if (Insert(target, single, Direction::Opposite(facing), this).IsEmpty())
        {
            item->SetCount(item->GetCount() - 1);
            if (item->IsEmpty())
            {
                ClearItem(i);
            }

            MarkDirty(m_level, targetPos, partnerPos);
            return true;
        }
    }

    return false;
}

bool HopperBlockEntity::SuckInItems(Level *level, AbstractContainer *hopper, const BlockPos &sourcePos, const AABB &captureBox,
                                    HopperBlockEntity *self, std::unique_ptr<CompoundContainer> *compoundChest)
{
    BlockPos partnerPos;
    AbstractContainer *source = GetContainerAt(level, sourcePos, &partnerPos, compoundChest);
    if (source == hopper)
    {
        return false;
    }

    if (source)
    {
        for (int i = 0; i < source->GetSize(); i++)
        {
            ItemInstance *item = source->GetItem(i);
            if (item->IsEmpty() || !CanExtract(source, i, *item))
            {
                continue;
            }

            ItemInstance single(*item);
            single.SetCount(1);
            if (Insert(hopper, single, nullptr, self).IsEmpty())
            {
                item->SetCount(item->GetCount() - 1);
                if (item->IsEmpty())
                {
                    source->ClearItem(i);
                }

                MarkDirty(level, sourcePos, partnerPos);
                return true;
            }
        }

        return false;
    }

    ServerLevel *serverLevel                             = (ServerLevel *) level;
    const std::vector<std::unique_ptr<Entity>> &entities = serverLevel->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i]->GetType() != eEntityType_Item || !entities[i]->GetAABB().Intersects(captureBox))
        {
            continue;
        }

        ItemEntity *itemEntity = (ItemEntity *) entities[i].get();
        if (itemEntity->GetItem().IsEmpty())
        {
            continue;
        }

        ItemInstance remaining = Insert(hopper, itemEntity->GetItem(), nullptr, self);
        if (remaining.IsEmpty())
        {
            itemEntity->SetItem(ItemInstance());
            serverLevel->RemoveEntity(itemEntity);
            return true;
        }

        if (remaining.GetCount() != itemEntity->GetItem().GetCount())
        {
            itemEntity->SetItem(remaining);
            serverLevel->SendEntity(itemEntity, nullptr);
        }
    }

    return false;
}

bool HopperBlockEntity::CanExtract(AbstractContainer *container, int slot, const ItemInstance &item)
{
    return !dynamic_cast<FurnaceBlockEntity *>(container) || slot == FurnaceContainer::s_resultSlot ||
           (slot == FurnaceContainer::s_fuelSlot && item.GetItem() == Item::ByName("bucket"));
}

void HopperBlockEntity::Save(CompoundTag *tag) const
{
    ItemTransferBlockEntity::Save(tag);
    tag->PutInt("TransferCooldown", m_cooldown);
}

void HopperBlockEntity::Load(const CompoundTag *tag)
{
    ItemTransferBlockEntity::Load(tag);
    m_cooldown = tag->GetInt("TransferCooldown");
}

// A hopper that just received its first item waits a full cooldown before passing it on
void HopperBlockEntity::OnFirstItemReceived(int64_t sourceTickedGameTime)
{
    m_cooldown = sourceTickedGameTime >= m_tickedGameTime ? 7 : 8;
}
