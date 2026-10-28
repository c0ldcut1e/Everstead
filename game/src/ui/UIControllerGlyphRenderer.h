#pragma once

#include "../input/ControllerGlyphs.h"

class UIControllerGlyphRenderer
{
public:
    static float GetWidth(eCONTROLLER_BUTTON button, float size);
    static float Render(eCONTROLLER_BUTTON button, float x, float y, float size);
};
