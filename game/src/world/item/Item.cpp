#include "Item.h"

#include "ItemRegistry.h"

Item::Item()
{
    m_handEquipped = false;
    m_id           = 0;
    m_maxStackSize = 64;
    m_maxDamage    = 0;
    m_attackDamage = 0;
    m_icon         = Icon();
}

Block *Item::GetBlock() const
{
    return nullptr;
}

Block *Item::GetLightBlock() const
{
    return GetBlock();
}

bool Item::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    (void) level;
    (void) pos;
    (void) face;
    return false;
}

void Item::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) look;
}

void Item::SpawnUseEffects(Level *level, const BlockPos &pos) const
{
    (void) level;
    (void) pos;
}

Item *Item::ById(uint32_t id)
{
    return ItemRegistry::GetInstance()->ById(id);
}

Item *Item::ByName(const ResourceLocation &location)
{
    return ItemRegistry::GetInstance()->Get(location);
}

uint32_t Item::GetId() const
{
    return m_id;
}

void Item::SetId(uint32_t id)
{
    m_id = id;
}

eUSE_ANIM Item::GetUseAnimation() const
{
    return eUseAnim_None;
}

int Item::GetUseDuration() const
{
    return 0;
}

float Item::GetDestroySpeed(Block *block) const
{
    (void) block;
    return 1.0f;
}

bool Item::CanHarvest(Block *block) const
{
    (void) block;
    return false;
}

int Item::GetMineDamage(Block *block) const
{
    (void) block;
    return 0;
}

int Item::GetHitDamage() const
{
    return 0;
}

int Item::GetArmorSlot() const
{
    return -1;
}

std::string Item::GetDescriptionId() const
{
    return GetResourceLocation().ToLanguageKey("item");
}

const ResourceLocation &Item::GetResourceLocation() const
{
    return ItemRegistry::GetInstance()->GetKey(m_id);
}

void Item::SetMaxStackSize(int maxStackSize)
{
    m_maxStackSize = maxStackSize;
}

int Item::GetMaxStackSize() const
{
    return m_maxStackSize;
}

void Item::SetMaxDamage(int maxDamage)
{
    m_maxDamage = maxDamage;
}

int Item::GetMaxDamage() const
{
    return m_maxDamage;
}

bool Item::IsDamageable() const
{
    return m_maxDamage > 0;
}

void Item::SetRepairItems(const std::vector<ResourceLocation> &repairItems)
{
    m_repairItems = repairItems;
}

bool Item::IsValidRepairItem(const Item *item) const
{
    for (size_t i = 0; i < m_repairItems.size(); i++)
    {
        if (m_repairItems[i] == item->GetResourceLocation())
        {
            return true;
        }
    }

    return false;
}

void Item::SetAttackDamage(int attackDamage)
{
    m_attackDamage = attackDamage;
}

int Item::GetAttackDamage() const
{
    return m_attackDamage;
}

void Item::SetHandEquipped(bool handEquipped)
{
    m_handEquipped = handEquipped;
}

bool Item::IsHandEquipped() const
{
    return m_handEquipped;
}

void Item::SetIconPath(const std::string &iconPath)
{
    m_iconPath = iconPath;
}

const std::string &Item::GetIconPath() const
{
    return m_iconPath;
}

void Item::SetIcon(const Icon &icon)
{
    m_icon = icon;
}

const Icon &Item::GetIcon() const
{
    return m_icon;
}
