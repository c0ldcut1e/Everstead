#pragma once

#include "../UITextureAtlas.h"
#include "UIComponent.h"

class Item;

class UIComponent_Hotbar : public UIComponent
{
public:
    UIComponent_Hotbar();

    void Render() override;
    static float GetTopY(int width, int height, float hudScale);
    static float GetY(int width, int height, float hudScale);
    static float GetItemNameBaselineY(int width, int height, float hudScale);
    static float GetItemNameTopY(int width, int height, float hudScale);
    static float GetItemNameFontScale(int width, int height, float hudScale);

    static const float s_popDuration;

private:
    static const int s_slotWidth = 20;
    static const int s_iconSize  = 16;
    static const float s_blockIconScale;
    static const int s_iconInset = 3;
    static const float s_bottomOffset;
    static const int s_hotbarPixelHeight = 22;

    UITextureAtlas::Region m_hotbarRegion;
    UITextureAtlas::Region m_selectedRegion;
    int m_nameSlot;
    Item *m_nameItem;
    float m_nameTime;
};
