#pragma once

#include <vector>

#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

// Slot 0 is the result and the grid follows row by row from slot 1
class CraftingContainer : public AbstractContainer
{
public:
    static const int s_resultSlot = 0;
    static const int s_gridSlot   = 1;

    CraftingContainer(int width, int height);
    ~CraftingContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    const ItemInstance *GetGridItem(int x, int y) const;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

private:
    int m_width;
    int m_height;
    std::vector<ItemInstance> m_items;
};
