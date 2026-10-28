#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Slot.h"

Slot::Slot()
{
    m_container = nullptr;
    m_index     = 0;
    m_x         = 0.0f;
    m_y         = 0.0f;
}

Slot::Slot(AbstractContainer *container, int index, float x, float y)
{
    m_container = container;
    m_index     = index;
    m_x         = x;
    m_y         = y;
}

AbstractContainer *Slot::GetContainer() const
{
    return m_container;
}

int Slot::GetIndex() const
{
    return m_index;
}

ItemInstance *Slot::GetItem() const
{
    if (!m_container)
    {
        return nullptr;
    }

    return m_container->GetItem(m_index);
}

void Slot::SetItem(const ItemInstance &item)
{
    if (!m_container)
    {
        return;
    }

    m_container->SetItem(m_index, item);
}

bool Slot::HasItem() const
{
    ItemInstance *item = GetItem();
    return item && !item->IsEmpty();
}

void Slot::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

float Slot::GetX() const
{
    return m_x;
}

float Slot::GetY() const
{
    return m_y;
}

bool Slot::Contains(float pointX, float pointY, float width, float height) const
{
    return pointX >= m_x && pointX < m_x + width && pointY >= m_y && pointY < m_y + height;
}

#endif
