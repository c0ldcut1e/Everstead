#pragma once

#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

class CompoundTag;

class ResonantSocketContainer : public AbstractContainer
{
public:
    ~ResonantSocketContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

    void Save(CompoundTag *tag) const;
    void Load(const CompoundTag *tag);

private:
    ItemInstance m_item;
};
