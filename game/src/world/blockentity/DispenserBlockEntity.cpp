#include "DispenserBlockEntity.h"

#include "../Level.h"

DispenserBlockEntity::DispenserBlockEntity(Level *level, const BlockPos &pos) : ItemTransferBlockEntity(level, pos, 3, 3)
{}

const char *DispenserBlockEntity::GetId() const
{
    return "dispenser";
}

int DispenserBlockEntity::GetRandomSlot()
{
    int slot  = -1;
    int count = 1;
    for (int i = 0; i < GetSize(); i++)
    {
        if (!GetItem(i)->IsEmpty() && m_level->GetRandom().NextInt(0, count++ - 1) == 0)
        {
            slot = i;
        }
    }

    return slot;
}
