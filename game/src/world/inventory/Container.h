#pragma once

#include <vector>

#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

class Container : public AbstractContainer
{
public:
    Container(int rows, int columns);
    ~Container() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

private:
    static constexpr int kMaxRows = 9;

    int m_columns;
    std::vector<ItemInstance> m_items;
};
