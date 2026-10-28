#include "MobSpawnerBlock.h"

MobSpawnerBlock::MobSpawnerBlock() : Block(true, "assets/texture/block/mob_spawner.png")
{
    SetTransparent(true);
}

const char *MobSpawnerBlock::GetBlockEntityId() const
{
    return "mob_spawner";
}
