#pragma once

#include <array>

#include "ControllerGlyphs.h"
#include "GameplayAction.h"

struct ControllerState
{
    float moveForward;
    float moveStrafe;
    double lookDeltaX;
    double lookDeltaY;
    float lookStickY;
    bool uiNavigateUpPressed;
    bool uiNavigateDownPressed;
    bool uiNavigateLeftPressed;
    bool uiNavigateRightPressed;
    bool uiActivatePressed;
    bool uiCancelPressed;
    bool forwardTapped;
    std::array<bool, g_controllerButtonCount> buttonDown;
    std::array<bool, g_controllerButtonCount> buttonPressed;
    bool anyInput;
    eCONTROLLER_TYPE type;

    ControllerState();
};

class ControllerBackend
{
public:
    virtual ~ControllerBackend() = default;

    virtual void Update(ControllerState &state, double deltaTime) = 0;
    virtual void SetLookSensitivity(double sensitivity);
    virtual double GetLookSensitivity() const;
};
