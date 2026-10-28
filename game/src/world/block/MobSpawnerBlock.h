#pragma once

#include "Block.h"

class MobSpawnerBlock : public Block
{
public:
    MobSpawnerBlock();

    const char *GetBlockEntityId() const override;
};
