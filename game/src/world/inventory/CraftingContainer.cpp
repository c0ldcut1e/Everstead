#include "CraftingContainer.h"

CraftingContainer::CraftingContainer(int width, int height)
{
    m_width  = width;
    m_height = height;
    m_items.resize((size_t) (width * height + 1));
}

int CraftingContainer::GetColumns() const
{
    return m_width;
}

int CraftingContainer::GetRows() const
{
    return m_height;
}

int CraftingContainer::GetSize() const
{
    return (int) m_items.size();
}

ItemInstance *CraftingContainer::GetItem(int index)
{
    if (index < 0 || index >= GetSize())
    {
        return nullptr;
    }

    return &m_items[(size_t) index];
}

const ItemInstance *CraftingContainer::GetItem(int index) const
{
    if (index < 0 || index >= GetSize())
    {
        return nullptr;
    }

    return &m_items[(size_t) index];
}

void CraftingContainer::SetItem(int index, const ItemInstance &item)
{
    if (index < 0 || index >= GetSize())
    {
        return;
    }

    m_items[(size_t) index] = item;
}

const ItemInstance *CraftingContainer::GetGridItem(int x, int y) const
{
    return &m_items[(size_t) (s_gridSlot + x + y * m_width)];
}

void CraftingContainer::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool CraftingContainer::IsEmpty() const
{
    for (size_t i = 0; i < m_items.size(); i++)
    {
        if (!m_items[i].IsEmpty())
        {
            return false;
        }
    }

    return true;
}
