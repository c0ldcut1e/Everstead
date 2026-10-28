#include "DropperBlockEntity.h"

#include "../../utils/Direction.h"

DropperBlockEntity::DropperBlockEntity(Level *level, const BlockPos &pos) : DispenserBlockEntity(level, pos)
{}

const char *DropperBlockEntity::GetId() const
{
    return "dropper";
}

// This returns false only when there is no container in front, a full one still swallows the attempt
bool DropperBlockEntity::InsertInFront(Direction *facing, int slot)
{
    BlockPos targetPos(m_pos.x + facing->dx, m_pos.y + facing->dy, m_pos.z + facing->dz);
    BlockPos partnerPos;
    AbstractContainer *target = GetContainerAt(m_level, targetPos, &partnerPos, &m_compoundChest);
    if (!target)
    {
        return false;
    }

    ItemInstance *item = GetItem(slot);
    ItemInstance single(*item);
    single.SetCount(1);
    if (Insert(target, single, Direction::Opposite(facing), this).IsEmpty())
    {
        item->SetCount(item->GetCount() - 1);
        MarkDirty(m_level, targetPos, partnerPos);
    }

    return true;
}
