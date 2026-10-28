#include "Container.h"

#include <algorithm>

Container::Container(int rows, int columns)
{
    m_columns = columns;
    m_items.resize((size_t) (std::clamp(rows, 1, kMaxRows) * columns));
}

int Container::GetColumns() const
{
    return m_columns;
}

int Container::GetRows() const
{
    return GetSize() / m_columns;
}

int Container::GetSize() const
{
    return (int) m_items.size();
}

ItemInstance *Container::GetItem(int index)
{
    if (index < 0 || index >= GetSize())
    {
        return nullptr;
    }

    return &m_items[(size_t) index];
}

const ItemInstance *Container::GetItem(int index) const
{
    if (index < 0 || index >= GetSize())
    {
        return nullptr;
    }

    return &m_items[(size_t) index];
}

void Container::SetItem(int index, const ItemInstance &item)
{
    if (index < 0 || index >= GetSize())
    {
        return;
    }

    m_items[(size_t) index] = item;
}

void Container::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool Container::IsEmpty() const
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
