#include "ArmorItem.h"

ArmorItem::ArmorItem()
{
    m_armorSlot = s_head;
    m_defense   = 0;
}

void ArmorItem::SetArmor(int slot, int defense, const std::string &layerTexture)
{
    m_armorSlot    = slot;
    m_defense      = defense;
    m_layerTexture = layerTexture;
}

int ArmorItem::GetArmorSlot() const
{
    return m_armorSlot;
}

int ArmorItem::GetDefense() const
{
    return m_defense;
}

const std::string &ArmorItem::GetLayerTexture() const
{
    return m_layerTexture;
}
