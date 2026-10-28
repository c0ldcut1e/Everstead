#pragma once

#include "../UITextureAtlas.h"
#include "UIComponent.h"

class UIComponent_Timer : public UIComponent
{
public:
    UIComponent_Timer(float x, float y, float size);

    void Render() override;

private:
    static const float s_animationSpeed;
    static const float s_iconSpacing;
    static const float s_alphaFalloff;
    UITextureAtlas::Region m_region;
    float m_x;
    float m_y;
    float m_size;
    float m_time;
};
