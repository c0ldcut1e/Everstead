#pragma once

#include <cstddef>
#include <cstdint>

enum eCONTROLLER_BUTTON : uint8_t
{
    eControllerButton_South         = 0,
    eControllerButton_East          = 1,
    eControllerButton_West          = 2,
    eControllerButton_North         = 3,
    eControllerButton_Start         = 4,
    eControllerButton_Back          = 5,
    eControllerButton_LeftShoulder  = 6,
    eControllerButton_RightShoulder = 7,
    eControllerButton_LeftTrigger   = 8,
    eControllerButton_RightTrigger  = 9,
    eControllerButton_DpadUp        = 10,
    eControllerButton_DpadDown      = 11,
    eControllerButton_DpadLeft      = 12,
    eControllerButton_DpadRight     = 13,
    eControllerButton_LeftStick     = 14,
    eControllerButton_RightStick    = 15,
    eControllerButton_Count         = 16,
};

enum eCONTROLLER_TYPE : uint8_t
{
    eControllerType_Generic     = 0,
    eControllerType_PlayStation = 1,
    eControllerType_Xbox        = 2,
};

const size_t g_controllerButtonCount = (size_t) eControllerButton_Count;

class ControllerGlyphs
{
public:
    static const char *GetIconPath(eCONTROLLER_BUTTON button, eCONTROLLER_TYPE type);
};
