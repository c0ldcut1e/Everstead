#pragma once

#include "../world/inventory/AbstractContainer.h"
#include "../world/item/ItemInstance.h"

class Slot
{
public:
    Slot();
    Slot(AbstractContainer *container, int index, float x, float y);

    AbstractContainer *GetContainer() const;
    int GetIndex() const;

    ItemInstance *GetItem() const;
    void SetItem(const ItemInstance &item);
    bool HasItem() const;

    void SetPosition(float x, float y);
    float GetX() const;
    float GetY() const;

    bool Contains(float pointX, float pointY, float width, float height) const;

private:
    AbstractContainer *m_container;
    int m_index;
    float m_x;
    float m_y;
};
