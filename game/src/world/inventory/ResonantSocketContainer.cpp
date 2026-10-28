#include "ResonantSocketContainer.h"

#include "../../tag/CompoundTag.h"
#include "../save/WorldSave.h"

int ResonantSocketContainer::GetColumns() const
{
    return 1;
}

int ResonantSocketContainer::GetRows() const
{
    return 1;
}

int ResonantSocketContainer::GetSize() const
{
    return 1;
}

ItemInstance *ResonantSocketContainer::GetItem(int index)
{
    return index == 0 ? &m_item : nullptr;
}

const ItemInstance *ResonantSocketContainer::GetItem(int index) const
{
    return index == 0 ? &m_item : nullptr;
}

void ResonantSocketContainer::SetItem(int index, const ItemInstance &item)
{
    if (index == 0)
    {
        m_item = item;
    }
}

void ResonantSocketContainer::ClearItem(int index)
{
    SetItem(index, ItemInstance());
}

bool ResonantSocketContainer::IsEmpty() const
{
    return m_item.IsEmpty();
}

void ResonantSocketContainer::Save(CompoundTag *tag) const
{
    if (!m_item.IsEmpty())
    {
        tag->Put("Item", WorldSave::CreateItemTag(m_item));
    }
}

void ResonantSocketContainer::Load(const CompoundTag *tag)
{
    m_item = tag->Contains("Item") ? WorldSave::ReadItemTag(tag->GetCompound("Item")) : ItemInstance();
}
