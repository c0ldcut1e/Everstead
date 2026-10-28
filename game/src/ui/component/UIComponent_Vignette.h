#pragma once

#include "../../rendering/Texture.h"
#include "UIComponent.h"

class UIComponent_Vignette : public UIComponent
{
public:
    UIComponent_Vignette();
    ~UIComponent_Vignette() override;

    void Render() override;

private:
    Texture *m_texture;
    float m_smoothedStrength;
};
