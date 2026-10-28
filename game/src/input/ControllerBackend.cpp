#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ControllerBackend.h"

ControllerState::ControllerState()
{
    moveForward            = 0.0f;
    moveStrafe             = 0.0f;
    lookDeltaX             = 0.0;
    lookDeltaY             = 0.0;
    lookStickY             = 0.0f;
    uiNavigateUpPressed    = false;
    uiNavigateDownPressed  = false;
    uiNavigateLeftPressed  = false;
    uiNavigateRightPressed = false;
    uiActivatePressed      = false;
    uiCancelPressed        = false;
    forwardTapped          = false;
    buttonDown             = {};
    buttonPressed          = {};
    anyInput               = false;
    type                   = eControllerType_Generic;
}

void ControllerBackend::SetLookSensitivity(double)
{}

double ControllerBackend::GetLookSensitivity() const
{
    return 100.0;
}

#endif
