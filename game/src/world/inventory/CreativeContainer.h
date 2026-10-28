#pragma once

#include <vector>

#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

class CreativeContainer : public AbstractContainer
{
public:
    CreativeContainer(int columns, int rows);
    ~CreativeContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

private:
    int m_columns;
    int m_rows;
    std::vector<ItemInstance> m_items;
};
