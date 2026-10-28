#include "ItemInstance.h"

#include <memory>
#include <vector>

#include "../../entity/Entity.h"
#include "../../rendering/Icon.h"
#include "../../tag/CompoundTag.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "../fracture/FractureTrait.h"
#include "../fracture/FractureTraits.h"
#include "Item.h"

static const Icon s_emptyIcon;

ItemInstance::ItemInstance()
{
    m_item     = nullptr;
    m_count    = 0;
    m_auxValue = 0;
    m_tag      = nullptr;
}

ItemInstance::ItemInstance(Item *item)
{
    m_item     = item;
    m_count    = 1;
    m_auxValue = 0;
    m_tag      = nullptr;
}

ItemInstance::ItemInstance(Item *item, int count)
{
    m_item     = item;
    m_count    = count;
    m_auxValue = 0;
    m_tag      = nullptr;
}

ItemInstance::ItemInstance(Item *item, int count, int auxValue)
{
    m_item     = item;
    m_count    = count;
    m_auxValue = auxValue;
    m_tag      = nullptr;
}

ItemInstance::ItemInstance(const ItemInstance &other)
{
    m_item     = other.m_item;
    m_count    = other.m_count;
    m_auxValue = other.m_auxValue;
    m_tag      = other.m_tag ? (CompoundTag *) other.m_tag->Clone().release() : nullptr;
}

ItemInstance &ItemInstance::operator=(const ItemInstance &other)
{
    if (this == &other)
    {
        return *this;
    }

    delete m_tag;
    m_item     = other.m_item;
    m_count    = other.m_count;
    m_auxValue = other.m_auxValue;
    m_tag      = other.m_tag ? (CompoundTag *) other.m_tag->Clone().release() : nullptr;
    return *this;
}

ItemInstance::~ItemInstance()
{
    delete m_tag;
}

Item *ItemInstance::GetItem() const
{
    return m_item;
}

bool ItemInstance::IsEmpty() const
{
    return m_item == nullptr || m_count <= 0;
}

int ItemInstance::GetCount() const
{
    return m_count;
}

void ItemInstance::SetCount(int count)
{
    m_count = count;
}

void ItemInstance::ConsumeOne()
{
    if (m_item && m_item->IsDamageable())
    {
        m_auxValue++;
        if (m_auxValue < m_item->GetMaxDamage())
        {
            return;
        }
        m_auxValue = 0;
    }
    m_count--;
}

void ItemInstance::HurtAndBreak(int amount, Entity *owner)
{
    if (m_tag && m_tag->GetBoolean("Unbreakable"))
    {
        return;
    }

    Random &random = owner->GetLevel()->GetRandom();

    bool damageable                        = m_item && m_item->IsDamageable();
    std::vector<FractureTrait *> fractures = FractureTraits::GetActive(*this);
    for (size_t i = 0; i < fractures.size(); i++)
    {
        amount = fractures[i]->ModifyWear(amount, &random);
    }

    for (int i = 0; i < amount && !IsEmpty(); i++)
    {
        ConsumeOne();
    }

    if (damageable && IsEmpty())
    {
        owner->GetLevel()->PlaySound("random.break", owner->GetPosition(), 0.8f, 0.8f + random.NextFloat() * 0.4f);
    }
}

int ItemInstance::GetAuxValue() const
{
    return m_auxValue;
}

void ItemInstance::SetAuxValue(int auxValue)
{
    m_auxValue = auxValue;
}

int ItemInstance::GetMaxStackSize() const
{
    return m_item ? m_item->GetMaxStackSize() : 0;
}

bool ItemInstance::IsStackable() const
{
    return GetMaxStackSize() > 1;
}

bool ItemInstance::IsSameItem(const ItemInstance &other) const
{
    return m_item != nullptr && m_item == other.m_item && m_auxValue == other.m_auxValue && m_tag == nullptr && other.m_tag == nullptr;
}

bool ItemInstance::HasTag() const
{
    return m_tag != nullptr;
}

CompoundTag *ItemInstance::GetTag() const
{
    return m_tag;
}

void ItemInstance::SetTag(CompoundTag *tag)
{
    if (m_tag == tag)
    {
        return;
    }

    delete m_tag;
    m_tag = tag;
}

bool ItemInstance::HasCustomName() const
{
    return m_tag && m_tag->Contains("display", Tag::eTagType_Compound) && m_tag->GetCompound("display")->Contains("Name", Tag::eTagType_String);
}

std::string ItemInstance::GetCustomName() const
{
    return HasCustomName() ? m_tag->GetCompound("display")->GetString("Name") : std::string();
}

void ItemInstance::SetCustomName(const std::string &name)
{
    if (!name.empty())
    {
        if (!m_tag)
        {
            m_tag = new CompoundTag();
        }

        if (!m_tag->Contains("display", Tag::eTagType_Compound))
        {
            m_tag->Put("display", std::unique_ptr<CompoundTag>(new CompoundTag()));
        }

        m_tag->GetCompound("display")->PutString("Name", name);
        return;
    }

    if (!HasCustomName())
    {
        return;
    }

    CompoundTag *display = m_tag->GetCompound("display");
    display->Remove("Name");
    if (display->IsEmpty())
    {
        m_tag->Remove("display");
    }

    if (m_tag->IsEmpty())
    {
        SetTag(nullptr);
    }
}

int ItemInstance::GetRepairCost() const
{
    return m_tag && m_tag->Contains("RepairCost", Tag::eTagType_Int) ? m_tag->GetInt("RepairCost") : 0;
}

void ItemInstance::SetRepairCost(int repairCost)
{
    if (!m_tag)
    {
        m_tag = new CompoundTag();
    }

    m_tag->PutInt("RepairCost", repairCost);
}

const Icon &ItemInstance::GetIcon() const
{
    return m_item ? m_item->GetIcon() : s_emptyIcon;
}
