#pragma once

#include "../../utils/Random.h"
#include "../../world/block/BlockPos.h"
#include "../Entity.h"

class Player;

class LeashKnot : public Entity
{
public:
    explicit LeashKnot(Level *level);
    ~LeashKnot() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;

    void Interact(Player *player);
    void Hurt();
    BlockPos GetFencePos() const;

    static LeashKnot *GetOrCreate(Level *level, const BlockPos &pos);
    static bool BindPlayerMobs(Player *player, const BlockPos &pos);

    static const eENTITY_TYPE s_type = eEntityType_LeashKnot;

private:
};
