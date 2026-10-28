#pragma once

#include "Item.h"

class Direction;
class Level;
class ServerPlayer;

class BoatItem : public Item
{
public:
    explicit BoatItem(int woodType);

    int GetWoodType() const;
    void Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const;

private:
    int m_woodType;
};
