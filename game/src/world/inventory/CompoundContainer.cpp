#include "CompoundContainer.h"

CompoundContainer::CompoundContainer(AbstractContainer *first, AbstractContainer *second)
{
    m_first  = first;
    m_second = second;
}

int CompoundContainer::GetColumns() const
{
    return m_first->GetColumns();
}

int CompoundContainer::GetRows() const
{
    return m_first->GetRows() + m_second->GetRows();
}

int CompoundContainer::GetSize() const
{
    return m_first->GetSize() + m_second->GetSize();
}

ItemInstance *CompoundContainer::GetItem(int index)
{
    return index >= m_first->GetSize() ? m_second->GetItem(index - m_first->GetSize()) : m_first->GetItem(index);
}

const ItemInstance *CompoundContainer::GetItem(int index) const
{
    return index >= m_first->GetSize() ? m_second->GetItem(index - m_first->GetSize()) : m_first->GetItem(index);
}

void CompoundContainer::SetItem(int index, const ItemInstance &item)
{
    if (index >= m_first->GetSize())
    {
        m_second->SetItem(index - m_first->GetSize(), item);
    }
    else
    {
        m_first->SetItem(index, item);
    }
}

void CompoundContainer::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool CompoundContainer::IsEmpty() const
{
    return m_first->IsEmpty() && m_second->IsEmpty();
}

AbstractContainer *CompoundContainer::GetFirst() const
{
    return m_first;
}

AbstractContainer *CompoundContainer::GetSecond() const
{
    return m_second;
}
