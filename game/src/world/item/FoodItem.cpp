#include "FoodItem.h"

FoodItem::FoodItem(int nutrition, float saturationModifier)
{
    m_nutrition          = nutrition;
    m_saturationModifier = saturationModifier;
    m_block              = nullptr;
    m_remainder          = nullptr;
    m_poisonChance       = 0.0f;
    m_poisonTicks        = 0;
    m_hungerChance       = 0.0f;
    m_hungerTicks        = 0;
}

FoodItem::FoodItem(int nutrition, float saturationModifier, Block *block)
{
    m_nutrition          = nutrition;
    m_saturationModifier = saturationModifier;
    m_block              = block;
    m_remainder          = nullptr;
    m_poisonChance       = 0.0f;
    m_poisonTicks        = 0;
    m_hungerChance       = 0.0f;
    m_hungerTicks        = 0;
}

Block *FoodItem::GetBlock() const
{
    return m_block;
}

eUSE_ANIM FoodItem::GetUseAnimation() const
{
    return eUseAnim_Eat;
}

int FoodItem::GetUseDuration() const
{
    return 32;
}

int FoodItem::GetNutrition() const
{
    return m_nutrition;
}

float FoodItem::GetSaturationModifier() const
{
    return m_saturationModifier;
}

Item *FoodItem::GetRemainder() const
{
    return m_remainder;
}

void FoodItem::SetRemainder(Item *remainder)
{
    m_remainder = remainder;
}

float FoodItem::GetPoisonChance() const
{
    return m_poisonChance;
}

int FoodItem::GetPoisonTicks() const
{
    return m_poisonTicks;
}

void FoodItem::SetPoison(float chance, int ticks)
{
    m_poisonChance = chance;
    m_poisonTicks  = ticks;
}

float FoodItem::GetHungerChance() const
{
    return m_hungerChance;
}

int FoodItem::GetHungerTicks() const
{
    return m_hungerTicks;
}

void FoodItem::SetHunger(float chance, int ticks)
{
    m_hungerChance = chance;
    m_hungerTicks  = ticks;
}
