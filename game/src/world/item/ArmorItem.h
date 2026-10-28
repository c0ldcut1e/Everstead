#pragma once

#include <string>

#include "Item.h"

class ArmorItem : public Item
{
public:
    ArmorItem();

    void SetArmor(int slot, int defense, const std::string &layerTexture);
    int GetArmorSlot() const override;
    int GetDefense() const;
    const std::string &GetLayerTexture() const;

    static const int s_head  = 0;
    static const int s_torso = 1;
    static const int s_legs  = 2;
    static const int s_feet  = 3;

private:
    int m_armorSlot;
    int m_defense;
    std::string m_layerTexture;
};
