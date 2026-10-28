#pragma once

#include "../block/Block.h"
#include "Item.h"

class DiggerItem : public Item
{
public:
    DiggerItem();

    void SetTier(Block::eTOOL_TYPE toolType, int level, float speed);
    float GetDestroySpeed(Block *block) const override;
    bool CanHarvest(Block *block) const override;
    int GetMineDamage(Block *block) const override;
    int GetHitDamage() const override;

private:
    Block::eTOOL_TYPE m_toolType;
    int m_level;
    float m_speed;
};
