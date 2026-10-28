#pragma once

#include "ItemTransferBlockEntity.h"

class DispenserBlockEntity : public ItemTransferBlockEntity
{
public:
    DispenserBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;

    int GetRandomSlot();
};
