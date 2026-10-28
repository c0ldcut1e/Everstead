#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SdlControllerBackend.h"

#include "../utils/Time.h"

#include <algorithm>
#include <cmath>

#include "../core/FrameProfiler.h"
#include "../core/Logger.h"
#include "../utils/math/Mth.h"

bool SdlControllerBackend::s_deviceListDirty = true;

void SdlControllerBackend::RequestDeviceRescan()
{
    s_deviceListDirty = true;
}

SdlControllerBackend::SdlControllerBackend()
{
    m_controller               = nullptr;
    m_previousUiNavigateUpDown = false;
    m_navigateRepeatTimers.fill(0.0);
    m_previousUiNavigateDownDown  = false;
    m_previousUiNavigateLeftDown  = false;
    m_previousUiNavigateRightDown = false;
    m_previousForwardDown         = false;
    m_previousButtonDown          = {};
    m_lookSensitivity             = 100.0;
    m_nextDetectTicks             = 0;
}

SdlControllerBackend::~SdlControllerBackend()
{
    CloseController();
}

void SdlControllerBackend::Update(ControllerState &state, double deltaTime)
{
    bool haveController = false;
    {
        FrameProfilerScope scope("Controller::Ensure");
        haveController = EnsureController();
    }

    if (!haveController)
    {
        ResetPreviousState();
        return;
    }

    {
        FrameProfilerScope scope("Controller::SdlUpdate");
        SDL_GameControllerUpdate();
    }

    const int leftStickDeadzone  = 8000;
    const int rightStickDeadzone = 8000;
    const int triggerDeadzone    = 12000;
    const float triggerThreshold = 0.5f;
    double lookSpeed             = kBaseLookSpeed * exp2((m_lookSensitivity - 100.0) * 0.03);

    state.moveStrafe  = NormalizeAxis(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX), leftStickDeadzone);
    state.moveForward = -NormalizeAxis(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY), leftStickDeadzone);
    state.lookStickY  = -NormalizeAxis(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTY), rightStickDeadzone);
    state.lookDeltaX +=
            NormalizeAxis(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTX), rightStickDeadzone) * lookSpeed * deltaTime;
    state.lookDeltaY +=
            -NormalizeAxis(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTY), rightStickDeadzone) * lookSpeed * deltaTime;

    static const SDL_GameControllerButton buttons[g_controllerButtonCount] = {SDL_CONTROLLER_BUTTON_A,
                                                                              SDL_CONTROLLER_BUTTON_B,
                                                                              SDL_CONTROLLER_BUTTON_X,
                                                                              SDL_CONTROLLER_BUTTON_Y,
                                                                              SDL_CONTROLLER_BUTTON_START,
                                                                              SDL_CONTROLLER_BUTTON_BACK,
                                                                              SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
                                                                              SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
                                                                              SDL_CONTROLLER_BUTTON_INVALID,
                                                                              SDL_CONTROLLER_BUTTON_INVALID,
                                                                              SDL_CONTROLLER_BUTTON_DPAD_UP,
                                                                              SDL_CONTROLLER_BUTTON_DPAD_DOWN,
                                                                              SDL_CONTROLLER_BUTTON_DPAD_LEFT,
                                                                              SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
                                                                              SDL_CONTROLLER_BUTTON_LEFTSTICK,
                                                                              SDL_CONTROLLER_BUTTON_RIGHTSTICK};
    std::array<bool, g_controllerButtonCount> buttonDown{};
    for (size_t i = 0; i < g_controllerButtonCount; i++)
    {
        buttonDown[i] = buttons[i] != SDL_CONTROLLER_BUTTON_INVALID && IsButtonDown(buttons[i]);
    }

    buttonDown[eControllerButton_LeftTrigger] =
            NormalizeTrigger(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT), triggerDeadzone) >= triggerThreshold;
    buttonDown[eControllerButton_RightTrigger] =
            NormalizeTrigger(SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT), triggerDeadzone) >= triggerThreshold;
    for (size_t i = 0; i < g_controllerButtonCount; i++)
    {
        state.buttonDown[i]    = buttonDown[i];
        state.buttonPressed[i] = buttonDown[i] && !m_previousButtonDown[i];
        state.anyInput         = state.anyInput || state.buttonPressed[i];
    }

    bool forwardDown             = state.moveForward >= kForwardTapThreshold;
    bool navigateUpDown          = buttonDown[eControllerButton_DpadUp] || state.moveForward >= 0.5f;
    bool navigateDownDown        = buttonDown[eControllerButton_DpadDown] || state.moveForward <= -0.5f;
    bool navigateLeftDown        = buttonDown[eControllerButton_DpadLeft] || state.moveStrafe <= -0.5f;
    bool navigateRightDown       = buttonDown[eControllerButton_DpadRight] || state.moveStrafe >= 0.5f;
    state.uiNavigateUpPressed    = UpdateNavigateRepeat(navigateUpDown, m_previousUiNavigateUpDown, m_navigateRepeatTimers[0], deltaTime);
    state.uiNavigateDownPressed  = UpdateNavigateRepeat(navigateDownDown, m_previousUiNavigateDownDown, m_navigateRepeatTimers[1], deltaTime);
    state.uiNavigateLeftPressed  = UpdateNavigateRepeat(navigateLeftDown, m_previousUiNavigateLeftDown, m_navigateRepeatTimers[2], deltaTime);
    state.uiNavigateRightPressed = UpdateNavigateRepeat(navigateRightDown, m_previousUiNavigateRightDown, m_navigateRepeatTimers[3], deltaTime);
    state.uiActivatePressed      = state.buttonPressed[eControllerButton_South];
    state.uiCancelPressed        = state.buttonPressed[eControllerButton_East];
    state.forwardTapped          = forwardDown && !m_previousForwardDown;
    state.anyInput = state.anyInput || state.moveForward != 0.0f || state.moveStrafe != 0.0f || state.lookDeltaX != 0.0 || state.lookDeltaY != 0.0;

    SDL_GameControllerType controllerType = SDL_GameControllerGetType(m_controller);
    if (controllerType == SDL_CONTROLLER_TYPE_PS3 || controllerType == SDL_CONTROLLER_TYPE_PS4 || controllerType == SDL_CONTROLLER_TYPE_PS5)
    {
        state.type = eControllerType_PlayStation;
    }
    else if (controllerType == SDL_CONTROLLER_TYPE_XBOX360 || controllerType == SDL_CONTROLLER_TYPE_XBOXONE)
    {
        state.type = eControllerType_Xbox;
    }

    m_previousButtonDown          = buttonDown;
    m_previousUiNavigateUpDown    = navigateUpDown;
    m_previousUiNavigateDownDown  = navigateDownDown;
    m_previousUiNavigateLeftDown  = navigateLeftDown;
    m_previousUiNavigateRightDown = navigateRightDown;
    m_previousForwardDown         = forwardDown;
}

void SdlControllerBackend::SetLookSensitivity(double sensitivity)
{
    m_lookSensitivity = std::clamp(sensitivity, 0.0, kMaxLookSensitivity);
}

double SdlControllerBackend::GetLookSensitivity() const
{
    return m_lookSensitivity;
}

bool SdlControllerBackend::EnsureController()
{
    if (m_controller && SDL_GameControllerGetAttached(m_controller) == SDL_TRUE)
    {
        return true;
    }

    if (m_controller)
    {
        Logger::LogInfo("Controller disconnected");
        CloseController();
    }

    if (!s_deviceListDirty)
    {
        uint32_t now = Time::GetMilliseconds();
        if (now < m_nextDetectTicks)
        {
            return false;
        }

        m_nextDetectTicks = now + s_detectIntervalMs;

        FrameProfilerScope scope("Controller::Detect");
        SDL_GameControllerUpdate();

        if (!s_deviceListDirty)
        {
            return false;
        }
    }

    s_deviceListDirty = false;

    int joystickCount = SDL_NumJoysticks();
    for (int i = 0; i < joystickCount; i++)
    {
        if (SDL_IsGameController(i) != SDL_TRUE)
        {
            continue;
        }

        m_controller = SDL_GameControllerOpen(i);
        if (m_controller)
        {
            const char *name = SDL_GameControllerName(m_controller);
            Logger::LogInfo("Controller connected: %s", name ? name : "unknown");
            return true;
        }
    }

    return false;
}

void SdlControllerBackend::CloseController()
{
    if (m_controller)
    {
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
    }

    ResetPreviousState();
}

bool SdlControllerBackend::IsButtonDown(SDL_GameControllerButton button) const
{
    return SDL_GameControllerGetButton(m_controller, button) != 0;
}

void SdlControllerBackend::ResetPreviousState()
{
    m_previousUiNavigateUpDown = false;
    m_navigateRepeatTimers.fill(0.0);
    m_previousUiNavigateDownDown  = false;
    m_previousUiNavigateLeftDown  = false;
    m_previousUiNavigateRightDown = false;
    m_previousForwardDown         = false;
    m_previousButtonDown.fill(false);
}

float SdlControllerBackend::NormalizeAxis(int value, int deadzone)
{
    float normalized = (float) value / 32767.0f;
    float magnitude  = fabsf(normalized);
    if (magnitude <= (float) deadzone / 32767.0f)
    {
        return 0.0f;
    }

    float sign      = normalized < 0.0f ? -1.0f : 1.0f;
    float threshold = (float) deadzone / 32767.0f;
    float scaled    = (magnitude - threshold) / (1.0f - threshold);
    return sign * std::clamp(scaled, 0.0f, 1.0f);
}

float SdlControllerBackend::NormalizeTrigger(int value, int deadzone)
{
    float normalized = (float) value / 32767.0f;
    float threshold  = (float) deadzone / 32767.0f;
    if (normalized <= threshold)
    {
        return 0.0f;
    }

    return std::clamp((normalized - threshold) / (1.0f - threshold), 0.0f, 1.0f);
}

// Held navigation fires once, then repeats after a delay so sliders and lists can be scrolled
bool SdlControllerBackend::UpdateNavigateRepeat(bool down, bool previousDown, double &timer, double deltaTime)
{
    if (!down)
    {
        timer = 0.0;
        return false;
    }

    if (!previousDown)
    {
        timer = s_navigateRepeatDelay;
        return true;
    }

    timer -= deltaTime;
    if (timer > 0.0)
    {
        return false;
    }

    timer += s_navigateRepeatRate;
    if (timer <= 0.0)
    {
        timer = s_navigateRepeatRate;
    }

    return true;
}

#endif
