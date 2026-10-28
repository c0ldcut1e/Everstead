#pragma once

#include "Item.h"
#include "ItemInstance.h"

class Direction;
class Level;
class Player;
class ServerPlayer;

class BucketItem : public Item
{
public:
    explicit BucketItem(Block *liquid);

    Block *GetLiquid() const;
    Block *GetLightBlock() const override;
    bool CanUse(Level *level, const BlockPos &pos, Direction *face) const;
    ItemInstance UpdateHeldItem(Player *player, Level *level, const BlockPos &pos) const;
    void Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const;
    void PlaceLiquid(Level *level, const BlockPos &target) const;

private:
    Block *m_liquid;
};
