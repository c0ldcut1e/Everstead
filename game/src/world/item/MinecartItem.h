#pragma once

#include "Item.h"

class MinecartItem : public Item
{
public:
    explicit MinecartItem(int minecartType);

    int GetMinecartType() const;
    bool CanUseOn(Level *level, const BlockPos &pos, Direction *face) const override;
    void UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const override;

private:
    int m_minecartType;
};
