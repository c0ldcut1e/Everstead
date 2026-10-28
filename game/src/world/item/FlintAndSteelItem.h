#pragma once

#include "Item.h"

class Direction;
class Level;
class ServerPlayer;

class FlintAndSteelItem : public Item
{
public:
    FlintAndSteelItem();

    bool CanUse(Level *level, const BlockPos &pos, Direction *face) const;
    void Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const;
};
