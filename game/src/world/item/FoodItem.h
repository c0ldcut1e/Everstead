#pragma once

#include "Item.h"

class FoodItem : public Item
{
public:
    FoodItem(int nutrition, float saturationModifier);
    FoodItem(int nutrition, float saturationModifier, Block *block);

    Block *GetBlock() const override;

    eUSE_ANIM GetUseAnimation() const override;
    int GetUseDuration() const override;
    int GetNutrition() const;
    float GetSaturationModifier() const;
    Item *GetRemainder() const;
    void SetRemainder(Item *remainder);
    float GetPoisonChance() const;
    int GetPoisonTicks() const;
    void SetPoison(float chance, int ticks);
    float GetHungerChance() const;
    int GetHungerTicks() const;
    void SetHunger(float chance, int ticks);

private:
    int m_nutrition;
    float m_saturationModifier;
    Block *m_block;
    Item *m_remainder;
    float m_poisonChance;
    int m_poisonTicks;
    float m_hungerChance;
    int m_hungerTicks;
};
