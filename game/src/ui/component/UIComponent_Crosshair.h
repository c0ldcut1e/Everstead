#pragma once

#include "../UITextureAtlas.h"
#include "UIComponent.h"

class UIComponent_Crosshair : public UIComponent
{
public:
    UIComponent_Crosshair();

    void Render();

private:
    UITextureAtlas::Region m_region;
};
