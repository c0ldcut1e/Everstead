#include "CreativeContainer.h"

CreativeContainer::CreativeContainer(int columns, int rows)
{
    m_columns = columns;
    m_rows    = rows;
    m_items.resize((size_t) (columns * rows));
}

int CreativeContainer::GetColumns() const
{
    return m_columns;
}

int CreativeContainer::GetRows() const
{
    return m_rows;
}

int CreativeContainer::GetSize() const
{
    return (int) m_items.size();
}

ItemInstance *CreativeContainer::GetItem(int index)
{
    return &m_items[(size_t) index];
}

const ItemInstance *CreativeContainer::GetItem(int index) const
{
    return &m_items[(size_t) index];
}

void CreativeContainer::SetItem(int index, const ItemInstance &item)
{
    m_items[(size_t) index] = item;
}

void CreativeContainer::ClearItem(int index)
{
    m_items[(size_t) index] = ItemInstance();
}

bool CreativeContainer::IsEmpty() const
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
