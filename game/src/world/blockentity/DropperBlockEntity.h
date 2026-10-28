#pragma once

#include "DispenserBlockEntity.h"

class Direction;

class DropperBlockEntity : public DispenserBlockEntity
{
public:
    DropperBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;

    bool InsertInFront(Direction *facing, int slot);
};
