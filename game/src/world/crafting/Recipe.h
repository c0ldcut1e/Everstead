#pragma once

#include "../inventory/CraftingContainer.h"
#include "../item/ItemInstance.h"

class Recipe
{
public:
    Recipe(const ItemInstance &result);
    virtual ~Recipe() = default;

    virtual bool Matches(const CraftingContainer &grid) const       = 0;
    virtual void FillGrid(CraftingContainer *grid, int cycle) const = 0;
    const ItemInstance &GetResult() const;

private:
    ItemInstance m_result;
};
