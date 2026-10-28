#pragma once

#include "../../rendering/Texture.h"
#include "UIComponent.h"

class UIComponent_WaterOverlay : public UIComponent
{
public:
    UIComponent_WaterOverlay();
    ~UIComponent_WaterOverlay() override;

    void Render() override;

private:
    static const float s_tileCount;
    static const float s_scrollDivisor;
    static const float s_opacity;

    Texture *m_texture;
    float m_scroll;
    float m_lastYaw;
};
