#pragma once

#include <string>

class CompoundTag;
class Entity;
class Icon;
class Item;

class ItemInstance
{
public:
    ItemInstance();
    explicit ItemInstance(Item *item);
    ItemInstance(Item *item, int count);
    ItemInstance(Item *item, int count, int auxValue);
    ItemInstance(const ItemInstance &other);
    ItemInstance &operator=(const ItemInstance &other);
    ~ItemInstance();

    Item *GetItem() const;
    bool IsEmpty() const;

    int GetCount() const;
    void SetCount(int count);
    void ConsumeOne();
    void HurtAndBreak(int amount, Entity *owner);

    int GetAuxValue() const;
    void SetAuxValue(int auxValue);

    int GetMaxStackSize() const;
    bool IsStackable() const;
    bool IsSameItem(const ItemInstance &other) const;

    bool HasTag() const;
    CompoundTag *GetTag() const;
    void SetTag(CompoundTag *tag);

    bool HasCustomName() const;
    // The name is a JSON text component and an empty one removes it
    std::string GetCustomName() const;
    void SetCustomName(const std::string &name);
    int GetRepairCost() const;
    void SetRepairCost(int repairCost);

    const Icon &GetIcon() const;

private:
    Item *m_item;
    int m_count;
    int m_auxValue;
    CompoundTag *m_tag;
};
