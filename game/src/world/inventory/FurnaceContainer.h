#pragma once

#include "../item/ItemInstance.h"
#include "AbstractContainer.h"

class CompoundTag;

class FurnaceContainer : public AbstractContainer
{
public:
    static const int s_inputSlot    = 0;
    static const int s_fuelSlot     = 1;
    static const int s_resultSlot   = 2;
    static const int s_slotCount    = 3;
    static const int s_burnInterval = 200;

    FurnaceContainer();
    ~FurnaceContainer() override = default;

    int GetColumns() const override;
    int GetRows() const override;
    int GetSize() const override;

    ItemInstance *GetItem(int index) override;
    const ItemInstance *GetItem(int index) const override;
    void SetItem(int index, const ItemInstance &item) override;
    void ClearItem(int index) override;

    bool IsEmpty() const override;

    bool Tick();
    bool IsLit() const;
    int GetLitTime() const;
    void SetLitTime(int litTime);
    int GetLitDuration() const;
    void SetLitDuration(int litDuration);
    int GetCookTime() const;
    void SetCookTime(int cookTime);

    void Save(CompoundTag *tag) const;
    void Load(const CompoundTag *tag);

private:
    bool CanBurn() const;
    void Burn();

    ItemInstance m_items[s_slotCount];
    int m_litTime;
    int m_litDuration;
    int m_cookTime;
};
