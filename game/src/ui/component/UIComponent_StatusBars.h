#pragma once

#include "../UITextureAtlas.h"
#include "UIComponent.h"

class BufferBuilder;

class UIComponent_StatusBars : public UIComponent
{
public:
    UIComponent_StatusBars();

    static float GetTopY(int width, int height, float hudScale);
    void Render() override;

private:
    void DrawSprite(BufferBuilder *builder, const UITextureAtlas::Region &region, float guiX, float guiY, float guiWidth);

    static inline const int s_hotbarWidth  = 182;
    static const int s_iconSize            = 9;
    static const int s_iconSpacing         = 8;
    static const int s_experienceYOffset   = -8;
    static const int s_firstRowYOffset     = -18;
    static const int s_secondRowYOffset    = -28;
    static const int s_lowHealth           = 4;
    static const int s_shakeSeedMultiplier = 312871;

    float m_originX;
    float m_originY;
    float m_scale;
};
