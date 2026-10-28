#ifndef EVERSTEAD_DEDICATED_SERVER

#include "InputManager.h"

#include <algorithm>
#include <cmath>

#include <SDL2/SDL.h>

#include "../core/Everstead.h"
#include "../core/FrameProfiler.h"
#include "../utils/math/Mth.h"

InputManager::InputManager(std::unique_ptr<ControllerBackend> controllerBackend)
{
    m_keyboardMoveForward         = false;
    m_keyboardMoveBackward        = false;
    m_keyboardMoveLeft            = false;
    m_keyboardMoveRight           = false;
    m_keyboardJump                = false;
    m_keyboardAttack              = false;
    m_keyboardUse                 = false;
    m_keyboardSprint              = false;
    m_keyboardSneak               = false;
    m_gameplayInputBlocked        = false;
    m_keyboardPlayerList          = false;
    m_f3Down                      = false;
    m_f3ComboUsed                 = false;
    m_controllerMoveForward       = 0.0f;
    m_controllerMoveStrafe        = 0.0f;
    m_pendingMoveForward          = 0.0f;
    m_pendingMoveStrafe           = 0.0f;
    m_forwardTapCount             = 0;
    m_lookDeltaX                  = 0.0;
    m_lookDeltaY                  = 0.0;
    m_pendingHotbarScroll         = 0;
    m_pendingHotbarSlot           = -1;
    m_pendingUiNavigateUpPress    = false;
    m_pendingUiNavigateDownPress  = false;
    m_pendingUiNavigateLeftPress  = false;
    m_pendingUiNavigateRightPress = false;
    m_pendingUiActivatePress      = false;
    m_pendingUiCancelPress        = false;
    m_pendingUiScrollDelta        = 0;
    m_uiTextInputFocused          = false;
    m_pendingUiTextInput          = {};
    m_pendingUiTextBackspacePress = false;
    m_pendingUiTextDeletePress    = false;
    m_pendingUiTextSubmitPress    = false;
    m_pendingUiTextCancelPress    = false;
    m_pendingUiTextCompletePress  = false;
    m_pendingUiTextEditKeys       = {};
    m_pendingUiTextReplacement    = L"";
    m_hasUiTextReplacement        = false;
    m_usingController             = false;
    m_controllerType              = eControllerType_Generic;
    m_virtualKeyboardActive       = false;
    m_uiTabDelta                  = 0;
    m_controllerButtonsPressed    = {};
    m_controllerButtonsDown       = {};
    m_controllerLookStickY        = 0.0f;
    m_controllerSneakHeld         = false;
    m_pendingControllerPresses    = {};
    m_pendingKeyboardHorizontal   = 0;
    m_pendingKeyboardVertical     = 0;
    m_controllerActionDown        = {};
    m_pendingActionPresses        = {};
    m_bindings                    = {};
    m_rebindTarget                = -1;
    m_controllerRebindTarget      = -1;
    m_controllerSneakToggled      = false;
    m_controllerSprintToggled     = false;
    m_keyboardSneakHeld           = false;
    m_keyboardToggleSprint        = false;
    m_keyboardToggleSneak         = false;
    m_controllerToggleSprint      = false;
    m_controllerToggleSneak       = true;
    m_controllerBindings          = {};
    m_controllerBackend           = std::move(controllerBackend);
    ResetBindingsToDefault();
    ResetControllerBindingsToDefault();
}

InputManager::~InputManager()
{}

void InputManager::Update(double deltaTime)
{
    m_controllerMoveForward = 0.0f;
    m_controllerMoveStrafe  = 0.0f;
    m_controllerActionDown.fill(false);
    m_uiTabDelta = 0;
    m_controllerButtonsPressed.fill(false);
    m_controllerButtonsDown.fill(false);
    m_controllerLookStickY = 0.0f;
    m_controllerSneakHeld  = false;

    if (m_controllerBackend)
    {
        FrameProfilerScope scope("Controller::Update");
        ControllerState state;
        m_controllerBackend->Update(state, deltaTime);
        m_usingController = m_usingController || state.anyInput;
        m_controllerType  = state.type;
        if (m_virtualKeyboardActive)
        {
            for (size_t i = 0; i < g_controllerButtonCount; i++)
            {
                m_pendingControllerPresses[i] = m_pendingControllerPresses[i] || state.buttonPressed[i];
            }

            m_pendingKeyboardHorizontal += (state.uiNavigateRightPressed ? 1 : 0) - (state.uiNavigateLeftPressed ? 1 : 0);
            m_pendingKeyboardVertical += (state.uiNavigateDownPressed ? 1 : 0) - (state.uiNavigateUpPressed ? 1 : 0);
            CaptureMoveSample();
            return;
        }

        if (m_controllerRebindTarget >= 0)
        {
            for (size_t i = 0; i < g_controllerButtonCount; i++)
            {
                if (state.buttonPressed[i])
                {
                    m_controllerBindings[(size_t) m_controllerRebindTarget] = (eCONTROLLER_BUTTON) i;
                    m_controllerRebindTarget                                = -1;
                    break;
                }
            }

            CaptureMoveSample();
            return;
        }

        m_controllerMoveForward    = state.moveForward;
        m_controllerMoveStrafe     = state.moveStrafe;
        m_controllerButtonsPressed = state.buttonPressed;
        m_controllerButtonsDown    = state.buttonDown;
        m_controllerLookStickY     = state.lookStickY;
        m_lookDeltaX += state.lookDeltaX;
        m_lookDeltaY += state.lookDeltaY;
        ApplyControllerBindings(state);
        m_uiTabDelta = (state.buttonPressed[eControllerButton_RightShoulder] ? 1 : 0) - (state.buttonPressed[eControllerButton_LeftShoulder] ? 1 : 0);
        m_pendingUiNavigateUpPress    = m_pendingUiNavigateUpPress || state.uiNavigateUpPressed;
        m_pendingUiNavigateDownPress  = m_pendingUiNavigateDownPress || state.uiNavigateDownPressed;
        m_pendingUiNavigateLeftPress  = m_pendingUiNavigateLeftPress || state.uiNavigateLeftPressed;
        m_pendingUiNavigateRightPress = m_pendingUiNavigateRightPress || state.uiNavigateRightPressed;
        m_pendingUiActivatePress      = m_pendingUiActivatePress || state.uiActivatePressed;
        m_pendingUiCancelPress        = m_pendingUiCancelPress || state.uiCancelPressed;
        if (state.forwardTapped)
        {
            m_forwardTapCount++;
        }
    }

    {
        FrameProfilerScope scope("Input::CaptureMoveSample");
        CaptureMoveSample();
    }
}

void InputManager::ApplyControllerBindings(const ControllerState &state)
{
    static const eGAMEPLAY_ACTION actions[g_controllerBindingCount] = {eGameplayAction_Jump,
                                                                       eGameplayAction_ToggleInventory,
                                                                       eGameplayAction_BreakBlock,
                                                                       eGameplayAction_PlaceBlock,
                                                                       eGameplayAction_Count,
                                                                       eGameplayAction_Count,
                                                                       eGameplayAction_TogglePerspective,
                                                                       eGameplayAction_Count,
                                                                       eGameplayAction_Count,
                                                                       eGameplayAction_Chat,
                                                                       eGameplayAction_PlayerList,
                                                                       eGameplayAction_Back,
                                                                       eGameplayAction_PickBlock,
                                                                       eGameplayAction_DropItem,
                                                                       eGameplayAction_ToggleDebugOverlay,
                                                                       eGameplayAction_ToggleProfiler,
                                                                       eGameplayAction_ToggleHitboxes,
                                                                       eGameplayAction_ToggleChunkBorders,
                                                                       eGameplayAction_Screenshot,
                                                                       eGameplayAction_Panorama,
                                                                       eGameplayAction_ToggleHud};
    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        eCONTROLLER_BUTTON button = m_controllerBindings[i];
        if (actions[i] == eGameplayAction_Count || button == eControllerButton_Count)
        {
            continue;
        }

        m_controllerActionDown[(size_t) actions[i]] = state.buttonDown[button];
        if (state.buttonPressed[button])
        {
            QueueActionPress(actions[i]);
        }
    }

    m_pendingHotbarScroll += (state.buttonPressed[m_controllerBindings[eControllerBinding_HotbarPrevious]] ? 1 : 0) -
                             (state.buttonPressed[m_controllerBindings[eControllerBinding_HotbarNext]] ? 1 : 0);
    bool sneakDown  = state.buttonDown[m_controllerBindings[eControllerBinding_Sneak]];
    bool sprintDown = state.buttonDown[m_controllerBindings[eControllerBinding_Sprint]];
    if (!m_gameplayInputBlocked && state.buttonPressed[m_controllerBindings[eControllerBinding_Sneak]])
    {
        m_controllerSneakToggled = !m_controllerSneakToggled;
    }

    if (!m_gameplayInputBlocked && state.buttonPressed[m_controllerBindings[eControllerBinding_Sprint]])
    {
        m_controllerSprintToggled = !m_controllerSprintToggled;
    }

    m_controllerActionDown[(size_t) eGameplayAction_Sneak]  = m_controllerToggleSneak ? m_controllerSneakToggled : sneakDown;
    m_controllerActionDown[(size_t) eGameplayAction_Sprint] = m_controllerToggleSprint ? m_controllerSprintToggled : sprintDown;
    m_controllerSneakHeld                                   = sneakDown;
}

void InputManager::SetToggleOptions(bool keyboardSprint, bool keyboardSneak, bool controllerSprint, bool controllerSneak)
{
    m_keyboardToggleSprint   = keyboardSprint;
    m_keyboardToggleSneak    = keyboardSneak;
    m_controllerToggleSprint = controllerSprint;
    m_controllerToggleSneak  = controllerSneak;
    m_keyboardSprint         = false;
    m_keyboardSneak          = false;
    ClearSneakToggle();
}

eCONTROLLER_BUTTON InputManager::GetControllerBinding(eCONTROLLER_BINDING binding) const
{
    return m_controllerBindings[binding];
}

void InputManager::SetControllerBinding(eCONTROLLER_BINDING binding, eCONTROLLER_BUTTON button)
{
    m_controllerBindings[binding] = button;
}

void InputManager::ResetControllerBindingsToDefault()
{
    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        m_controllerBindings[i] = s_defaultControllerBindings[i];
    }
}

void InputManager::BeginControllerRebind(eCONTROLLER_BINDING binding)
{
    m_controllerRebindTarget = (int) binding;
}

void InputManager::CancelControllerRebind()
{
    m_controllerRebindTarget = -1;
}

bool InputManager::IsControllerRebinding() const
{
    return m_controllerRebindTarget >= 0;
}

void InputManager::Clear()
{
    m_keyboardMoveForward  = false;
    m_keyboardMoveBackward = false;
    m_keyboardMoveLeft     = false;
    m_keyboardMoveRight    = false;
    m_keyboardJump         = false;
    m_keyboardAttack       = false;
    m_keyboardUse          = false;
    m_keyboardSprint       = m_keyboardToggleSprint && m_keyboardSprint;
    m_keyboardSneak        = m_keyboardToggleSneak && m_keyboardSneak;
    m_keyboardSneakHeld    = false;
    m_keyboardPlayerList   = false;

    m_controllerMoveForward = 0.0f;
    m_controllerMoveStrafe  = 0.0f;
    m_controllerButtonsPressed.fill(false);
    m_controllerButtonsDown.fill(false);
    m_controllerLookStickY = 0.0f;
    m_pendingMoveForward   = 0.0f;
    m_pendingMoveStrafe    = 0.0f;
    m_forwardTapCount      = 0;

    m_lookDeltaX                  = 0.0;
    m_lookDeltaY                  = 0.0;
    m_pendingHotbarScroll         = 0;
    m_pendingHotbarSlot           = -1;
    m_pendingUiNavigateUpPress    = false;
    m_pendingUiNavigateDownPress  = false;
    m_pendingUiNavigateLeftPress  = false;
    m_pendingUiNavigateRightPress = false;
    m_pendingUiActivatePress      = false;
    m_pendingUiCancelPress        = false;
    m_pendingUiScrollDelta        = 0;
    SetUiTextInputFocused(false);
    m_pendingUiTextInput.clear();
    m_pendingUiTextBackspacePress = false;
    m_pendingUiTextDeletePress    = false;
    m_pendingUiTextSubmitPress    = false;
    m_pendingUiTextCancelPress    = false;
    m_pendingUiTextCompletePress  = false;
    m_pendingUiTextEditKeys       = {};

    m_controllerActionDown.fill(false);
    m_pendingActionPresses.fill(false);
}

void InputManager::OnKeyPressed(int key)
{
    if (m_usingController)
    {
        ClearSneakToggle();
    }

    m_usingController = false;
    if (m_uiTextInputFocused)
    {
        SDL_Keymod modifiers = SDL_GetModState();
        bool control         = (modifiers & (KMOD_CTRL | KMOD_GUI)) != 0;
        bool editKey         = key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT || key == SDL_SCANCODE_HOME || key == SDL_SCANCODE_END ||
                               key == SDL_SCANCODE_BACKSPACE || key == SDL_SCANCODE_DELETE;
        bool shortcutKey     = control && (key == SDL_SCANCODE_A || key == SDL_SCANCODE_C || key == SDL_SCANCODE_X || key == SDL_SCANCODE_V);
        if ((editKey || shortcutKey) && m_pendingUiTextEditKeys.size() < s_maxPendingTextEditKeys)
        {
            m_pendingUiTextEditKeys.push_back({key, (modifiers & KMOD_SHIFT) != 0, control});
        }

        if (key == SDL_SCANCODE_BACKSPACE)
        {
            m_pendingUiTextBackspacePress = true;
            return;
        }

        if (key == SDL_SCANCODE_DELETE)
        {
            m_pendingUiTextDeletePress = true;
            return;
        }

        if (key == SDL_SCANCODE_LEFT)
        {
            m_pendingUiNavigateLeftPress = true;
            return;
        }

        if (key == SDL_SCANCODE_RIGHT)
        {
            m_pendingUiNavigateRightPress = true;
            return;
        }

        if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER)
        {
            m_pendingUiTextSubmitPress = true;
            return;
        }

        if (key == SDL_SCANCODE_ESCAPE)
        {
            m_pendingUiTextCancelPress = true;
            return;
        }

        if (key == SDL_SCANCODE_TAB)
        {
            m_pendingUiTextCompletePress = true;
            return;
        }

        if (key != SDL_SCANCODE_UP && key != SDL_SCANCODE_DOWN)
        {
            return;
        }
    }

    if (m_rebindTarget >= 0)
    {
        if (key != SDL_SCANCODE_ESCAPE)
        {
            m_bindings[(size_t) m_rebindTarget] = key;
        }

        m_rebindTarget = -1;
        return;
    }

    for (size_t binding = 0; binding < g_keyBindingCount; binding++)
    {
        if (m_bindings[binding] == key)
        {
            ActivateBinding(binding);
        }
    }

    if (key == SDL_SCANCODE_ESCAPE)
    {
        QueueActionPress(eGameplayAction_Back);
    }

    if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_9)
    {
        m_pendingHotbarSlot = key - SDL_SCANCODE_1;
    }

    if (key == SDL_SCANCODE_UP)
    {
        m_pendingUiNavigateUpPress = true;
    }

    if (key == SDL_SCANCODE_DOWN)
    {
        m_pendingUiNavigateDownPress = true;
    }

    if (key == SDL_SCANCODE_LEFT)
    {
        m_pendingUiNavigateLeftPress = true;
    }

    if (key == SDL_SCANCODE_RIGHT)
    {
        m_pendingUiNavigateRightPress = true;
    }

    if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER)
    {
        m_pendingUiActivatePress = true;
    }
}

void InputManager::OnKeyReleased(int key)
{
    for (size_t binding = 0; binding < g_keyBindingCount; binding++)
    {
        if (m_bindings[binding] == key)
        {
            DeactivateBinding(binding);
        }
    }
}

void InputManager::OnMouseButtonPressed(int button)
{
    if (m_usingController)
    {
        ClearSneakToggle();
    }

    m_usingController = false;
    int inputCode     = s_mouseButtonInputBase + button;
    if (m_rebindTarget >= 0)
    {
        m_bindings[(size_t) m_rebindTarget] = inputCode;
        m_rebindTarget                      = -1;
        return;
    }

    for (size_t binding = 0; binding < g_keyBindingCount; binding++)
    {
        if (m_bindings[binding] == inputCode)
        {
            ActivateBinding(binding);
        }
    }
}

void InputManager::OnMouseButtonReleased(int button)
{
    int inputCode = s_mouseButtonInputBase + button;
    for (size_t binding = 0; binding < g_keyBindingCount; binding++)
    {
        if (m_bindings[binding] == inputCode)
        {
            DeactivateBinding(binding);
        }
    }
}

void InputManager::OnMouseMoved(double dx, double dy, bool look)
{
    m_usingController = m_usingController && dx == 0.0 && dy == 0.0;
    if (look)
    {
        m_lookDeltaX += dx;
        m_lookDeltaY += dy;
    }
}

void InputManager::OnMouseWheel(int wheelY)
{
    m_pendingUiScrollDelta += wheelY;
    m_pendingHotbarScroll += wheelY;
}

void InputManager::OnTextInput(const std::string &text)
{
    if (!m_uiTextInputFocused)
    {
        return;
    }

    m_pendingUiTextInput += text;
}

bool InputManager::IsActionDown(eGAMEPLAY_ACTION action) const
{
    if (action == eGameplayAction_Jump)
    {
        return m_keyboardJump || m_controllerActionDown[(size_t) action];
    }

    // Holding to repeat break is controller only, the mouse has to click each time
    if (action == eGameplayAction_BreakBlock)
    {
        return m_controllerActionDown[(size_t) action];
    }

    if (action == eGameplayAction_PlaceBlock)
    {
        return m_keyboardUse || m_controllerActionDown[(size_t) action];
    }

    if (action == eGameplayAction_PlayerList)
    {
        return m_keyboardPlayerList || m_controllerActionDown[(size_t) action];
    }

    if (action == eGameplayAction_Sprint)
    {
        return m_keyboardSprint || (m_usingController && m_controllerActionDown[(size_t) action]);
    }

    if (action == eGameplayAction_Sneak)
    {
        // The controller toggle only counts while the controller is the active device
        return m_keyboardSneak || (m_usingController && m_controllerActionDown[(size_t) action]);
    }

    return m_controllerActionDown[(size_t) action];
}

bool InputManager::IsSneakHeld() const
{
    return m_keyboardSneakHeld || m_controllerSneakHeld;
}

bool InputManager::IsSneakToggled() const
{
    return (m_keyboardToggleSneak && m_keyboardSneak) || (m_controllerToggleSneak && m_controllerSneakToggled);
}

void InputManager::ClearSneakToggle()
{
    m_controllerSneakToggled                                = false;
    m_controllerSprintToggled                               = false;
    m_controllerActionDown[(size_t) eGameplayAction_Sneak]  = false;
    m_controllerActionDown[(size_t) eGameplayAction_Sprint] = false;
}

void InputManager::SetGameplayInputBlocked(bool blocked)
{
    m_gameplayInputBlocked = blocked;
}

bool InputManager::IsUseItemHeld() const
{
    return m_keyboardUse || m_controllerActionDown[(size_t) eGameplayAction_PlaceBlock];
}

bool InputManager::IsDestroyHeld() const
{
    return m_keyboardAttack || m_controllerActionDown[(size_t) eGameplayAction_BreakBlock];
}

bool InputManager::ConsumeActionPress(eGAMEPLAY_ACTION action)
{
    size_t index                  = (size_t) action;
    bool pressed                  = m_pendingActionPresses[index];
    m_pendingActionPresses[index] = false;
    return pressed;
}

void InputManager::ConsumeLocalPlayerTickInputs(float *moveForward, float *moveStrafe, bool *jumpHeld, bool *jumpPressed)
{
    float currentMoveForward = GetMoveForward();
    float currentMoveStrafe  = GetMoveStrafe();
    bool usePendingMove      = fabsf(currentMoveForward) < 0.0001f && fabsf(currentMoveStrafe) < 0.0001f &&
                               (fabsf(m_pendingMoveForward) >= 0.0001f || fabsf(m_pendingMoveStrafe) >= 0.0001f);
    if (moveForward)
    {
        *moveForward = usePendingMove ? m_pendingMoveForward : currentMoveForward;
    }

    if (moveStrafe)
    {
        *moveStrafe = usePendingMove ? m_pendingMoveStrafe : currentMoveStrafe;
    }

    if (jumpHeld)
    {
        *jumpHeld = IsActionDown(eGameplayAction_Jump);
    }

    if (jumpPressed)
    {
        *jumpPressed = ConsumeActionPress(eGameplayAction_Jump);
    }

    if (usePendingMove)
    {
        m_pendingMoveForward = 0.0f;
        m_pendingMoveStrafe  = 0.0f;
    }
}

float InputManager::GetMoveForward() const
{
    float keyboard = 0.0f;
    if (m_keyboardMoveForward)
    {
        keyboard += 1.0f;
    }

    if (m_keyboardMoveBackward)
    {
        keyboard -= 1.0f;
    }

    return std::clamp(keyboard + m_controllerMoveForward, -1.0f, 1.0f);
}

float InputManager::GetMoveStrafe() const
{
    float keyboard = 0.0f;
    if (m_keyboardMoveRight)
    {
        keyboard += 1.0f;
    }

    if (m_keyboardMoveLeft)
    {
        keyboard -= 1.0f;
    }

    return std::clamp(keyboard + m_controllerMoveStrafe, -1.0f, 1.0f);
}

void InputManager::ConsumeLookDelta(double *dx, double *dy)
{
    if (dx)
    {
        *dx = m_lookDeltaX;
    }

    if (dy)
    {
        *dy = m_lookDeltaY;
    }

    m_lookDeltaX = 0.0;
    m_lookDeltaY = 0.0;
}

bool InputManager::ConsumeUiNavigateUpPress()
{
    bool pressed               = m_pendingUiNavigateUpPress;
    m_pendingUiNavigateUpPress = false;
    return pressed;
}

bool InputManager::ConsumeUiNavigateDownPress()
{
    bool pressed                 = m_pendingUiNavigateDownPress;
    m_pendingUiNavigateDownPress = false;
    return pressed;
}

bool InputManager::ConsumeUiNavigateLeftPress()
{
    bool pressed                 = m_pendingUiNavigateLeftPress;
    m_pendingUiNavigateLeftPress = false;
    return pressed;
}

bool InputManager::ConsumeUiNavigateRightPress()
{
    bool pressed                  = m_pendingUiNavigateRightPress;
    m_pendingUiNavigateRightPress = false;
    return pressed;
}

bool InputManager::ConsumeUiActivatePress()
{
    bool pressed             = m_pendingUiActivatePress;
    m_pendingUiActivatePress = false;
    return pressed;
}

bool InputManager::ConsumeUiCancelPress()
{
    bool pressed           = m_pendingUiCancelPress;
    m_pendingUiCancelPress = false;
    return pressed;
}

int InputManager::ConsumeHotbarScroll()
{
    int delta             = m_pendingHotbarScroll;
    m_pendingHotbarScroll = 0;
    return delta;
}

int InputManager::ConsumeHotbarSlot()
{
    int slot            = m_pendingHotbarSlot;
    m_pendingHotbarSlot = -1;
    return slot;
}

int InputManager::ConsumeUiScrollDelta()
{
    int delta              = m_pendingUiScrollDelta;
    m_pendingUiScrollDelta = 0;
    return delta;
}

void InputManager::SetUiTextInputFocused(bool focused)
{
    if (m_uiTextInputFocused == focused)
    {
        return;
    }

    m_uiTextInputFocused = focused;
    if (m_uiTextInputFocused)
    {
        SDL_StartTextInput();
    }
    else
    {
        SDL_StopTextInput();
        m_pendingUiTextInput.clear();
        m_pendingUiTextBackspacePress = false;
        m_pendingUiTextDeletePress    = false;
        m_pendingUiTextSubmitPress    = false;
        m_pendingUiTextCancelPress    = false;
        m_pendingUiTextCompletePress  = false;
        m_pendingUiTextEditKeys       = {};
    }
}

bool InputManager::IsUiTextInputFocused() const
{
    return m_uiTextInputFocused;
}

std::string InputManager::ConsumeUiTextInput()
{
    std::string text = m_pendingUiTextInput;
    m_pendingUiTextInput.clear();
    return text;
}

bool InputManager::ConsumeUiTextBackspacePress()
{
    bool pressed                  = m_pendingUiTextBackspacePress;
    m_pendingUiTextBackspacePress = false;
    return pressed;
}

bool InputManager::ConsumeUiTextDeletePress()
{
    bool pressed               = m_pendingUiTextDeletePress;
    m_pendingUiTextDeletePress = false;
    return pressed;
}

bool InputManager::ConsumeUiTextSubmitPress()
{
    bool pressed               = m_pendingUiTextSubmitPress;
    m_pendingUiTextSubmitPress = false;
    return pressed;
}

void InputManager::ConsumeUiTextEditKeys(std::vector<TextEditKey> *keys)
{
    keys->swap(m_pendingUiTextEditKeys);
    m_pendingUiTextEditKeys.clear();
}

void InputManager::InjectUiTextSubmit()
{
    m_pendingUiTextSubmitPress = true;
}

void InputManager::SetUiTextReplacement(const std::wstring &text)
{
    m_pendingUiTextReplacement = text;
    m_hasUiTextReplacement     = true;
}

bool InputManager::ConsumeUiTextReplacement(std::wstring *text)
{
    if (!m_hasUiTextReplacement)
    {
        return false;
    }

    *text                  = m_pendingUiTextReplacement;
    m_hasUiTextReplacement = false;
    return true;
}

void InputManager::InjectUiTextCancel()
{
    m_pendingUiTextCancelPress = true;
}

void InputManager::GetControllerStick(float *x, float *y) const
{
    *x = m_controllerMoveStrafe;
    *y = m_controllerMoveForward;
}

bool InputManager::IsControllerButtonPressed(eCONTROLLER_BUTTON button) const
{
    return m_controllerButtonsPressed[button];
}

bool InputManager::IsControllerButtonDown(eCONTROLLER_BUTTON button) const
{
    return m_controllerButtonsDown[button];
}

float InputManager::GetControllerLookStickY() const
{
    return m_controllerLookStickY;
}

bool InputManager::IsControllerButtonBound(eCONTROLLER_BUTTON button) const
{
    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        if (m_controllerBindings[i] == button)
        {
            return true;
        }
    }

    return false;
}

bool InputManager::IsUsingController() const
{
    return m_usingController;
}

eCONTROLLER_TYPE InputManager::GetControllerType() const
{
    return m_controllerType;
}

void InputManager::SetVirtualKeyboardActive(bool active)
{
    m_virtualKeyboardActive = active;
    m_pendingControllerPresses.fill(false);
    m_pendingKeyboardHorizontal = 0;
    m_pendingKeyboardVertical   = 0;
}

bool InputManager::ConsumeControllerButtonPress(eCONTROLLER_BUTTON button)
{
    bool pressed                       = m_pendingControllerPresses[button];
    m_pendingControllerPresses[button] = false;
    return pressed;
}

void InputManager::ConsumeVirtualKeyboardNavigation(int *horizontal, int *vertical)
{
    *horizontal                 = m_pendingKeyboardHorizontal;
    *vertical                   = m_pendingKeyboardVertical;
    m_pendingKeyboardHorizontal = 0;
    m_pendingKeyboardVertical   = 0;
}

int InputManager::ConsumeUiTabDelta()
{
    int delta    = m_uiTabDelta;
    m_uiTabDelta = 0;
    return delta;
}

bool InputManager::ConsumeUiTextCompletePress()
{
    bool pressed                 = m_pendingUiTextCompletePress;
    m_pendingUiTextCompletePress = false;
    return pressed;
}

bool InputManager::ConsumeUiTextCancelPress()
{
    bool pressed               = m_pendingUiTextCancelPress;
    m_pendingUiTextCancelPress = false;
    return pressed;
}

void InputManager::SetControllerLookSensitivity(double sensitivity)
{
    if (m_controllerBackend)
    {
        m_controllerBackend->SetLookSensitivity(sensitivity);
    }
}

double InputManager::GetControllerLookSensitivity() const
{
    if (m_controllerBackend)
    {
        return m_controllerBackend->GetLookSensitivity();
    }

    return 100.0;
}

void InputManager::QueueActionPress(eGAMEPLAY_ACTION action)
{
    m_pendingActionPresses[(size_t) action] = true;
}

int InputManager::ConsumeForwardTapCount()
{
    int taps          = m_forwardTapCount;
    m_forwardTapCount = 0;
    return taps;
}

int InputManager::GetDefaultBinding(eKEY_BINDING binding)
{
    switch (binding)
    {
        case eKeyBinding_Forward:
            return SDL_SCANCODE_W;
        case eKeyBinding_Back:
            return SDL_SCANCODE_S;
        case eKeyBinding_Left:
            return SDL_SCANCODE_A;
        case eKeyBinding_Right:
            return SDL_SCANCODE_D;
        case eKeyBinding_Jump:
            return SDL_SCANCODE_SPACE;
        case eKeyBinding_Sneak:
            return SDL_SCANCODE_LSHIFT;
        case eKeyBinding_Sprint:
            return SDL_SCANCODE_LCTRL;
        case eKeyBinding_Attack:
            return s_mouseButtonInputBase + SDL_BUTTON_LEFT;
        case eKeyBinding_Use:
            return s_mouseButtonInputBase + SDL_BUTTON_RIGHT;
        case eKeyBinding_Perspective:
            return SDL_SCANCODE_F5;
        case eKeyBinding_PlayerList:
            return SDL_SCANCODE_TAB;
        case eKeyBinding_Chat:
            return SDL_SCANCODE_T;
        case eKeyBinding_Command:
            return SDL_SCANCODE_SLASH;
        case eKeyBinding_Inventory:
            return SDL_SCANCODE_E;
        case eKeyBinding_PickBlock:
            return s_mouseButtonInputBase + SDL_BUTTON_MIDDLE;
        case eKeyBinding_Drop:
            return SDL_SCANCODE_Q;
        case eKeyBinding_ViewRecipe:
            return SDL_SCANCODE_R;
        case eKeyBinding_DebugMenu:
            return SDL_SCANCODE_F3;
        case eKeyBinding_DebugHitboxes:
            return SDL_SCANCODE_B;
        case eKeyBinding_DebugChunkBorders:
            return SDL_SCANCODE_G;
        case eKeyBinding_DebugAdvancedTooltips:
            return SDL_SCANCODE_H;
        case eKeyBinding_DebugReloadChunks:
            return SDL_SCANCODE_A;
        case eKeyBinding_Screenshot:
        case eKeyBinding_Panorama:
            return SDL_SCANCODE_F2;
        case eKeyBinding_HideHud:
            return SDL_SCANCODE_F1;
        default:
            return 0;
    }
}

void InputManager::ResetBindingsToDefault()
{
    for (size_t i = 0; i < g_keyBindingCount; i++)
    {
        m_bindings[i] = GetDefaultBinding((eKEY_BINDING) i);
    }
}

int InputManager::GetBinding(eKEY_BINDING binding) const
{
    return m_bindings[(size_t) binding];
}

void InputManager::SetBinding(eKEY_BINDING binding, int inputCode)
{
    m_bindings[(size_t) binding] = inputCode;
}

void InputManager::BeginRebind(eKEY_BINDING binding)
{
    m_rebindTarget = (int) binding;
}

void InputManager::CancelRebind()
{
    m_rebindTarget = -1;
}

bool InputManager::IsRebinding() const
{
    return m_rebindTarget >= 0;
}

eKEY_BINDING InputManager::GetRebindTarget() const
{
    return (eKEY_BINDING) (m_rebindTarget < 0 ? 0 : m_rebindTarget);
}

void InputManager::ActivateBinding(size_t binding)
{
    switch (binding)
    {
        case eKeyBinding_Forward:
            m_keyboardMoveForward = true;
            m_forwardTapCount++;
            CaptureMoveSample();
            break;
        case eKeyBinding_Back:
            m_keyboardMoveBackward = true;
            CaptureMoveSample();
            break;
        case eKeyBinding_Left:
            m_keyboardMoveLeft = true;
            CaptureMoveSample();
            break;
        case eKeyBinding_Right:
            m_keyboardMoveRight = true;
            CaptureMoveSample();
            break;
        case eKeyBinding_Jump:
            m_keyboardJump = true;
            QueueActionPress(eGameplayAction_Jump);
            break;
        case eKeyBinding_Sneak:
            m_keyboardSneak     = !m_keyboardToggleSneak || !m_keyboardSneak;
            m_keyboardSneakHeld = true;
            ClearSneakToggle();
            break;
        case eKeyBinding_Sprint:
            m_keyboardSprint = !m_keyboardToggleSprint || !m_keyboardSprint;
            break;
        case eKeyBinding_Attack:
            m_keyboardAttack = true;
            QueueActionPress(eGameplayAction_BreakBlock);
            break;
        case eKeyBinding_Use:
            m_keyboardUse = true;
            QueueActionPress(eGameplayAction_PlaceBlock);
            break;
        case eKeyBinding_Perspective:
            QueueActionPress(eGameplayAction_TogglePerspective);
            break;
        case eKeyBinding_PlayerList:
            m_keyboardPlayerList = true;
            QueueActionPress(eGameplayAction_PlayerList);
            break;
        case eKeyBinding_Chat:
            QueueActionPress(eGameplayAction_Chat);
            break;
        case eKeyBinding_Command:
            QueueActionPress(eGameplayAction_Command);
            break;
        case eKeyBinding_Inventory:
            QueueActionPress(eGameplayAction_ToggleInventory);
            break;
        case eKeyBinding_PickBlock:
            QueueActionPress(eGameplayAction_PickBlock);
            break;
        case eKeyBinding_Drop:
            QueueActionPress((SDL_GetModState() & KMOD_CTRL) != 0 ? eGameplayAction_DropStack : eGameplayAction_DropItem);
            break;
        case eKeyBinding_DebugMenu:
            m_f3Down      = true;
            m_f3ComboUsed = false;
            break;
        case eKeyBinding_DebugHitboxes:
            if (m_f3Down)
            {
                QueueActionPress(eGameplayAction_ToggleHitboxes);
                m_f3ComboUsed = true;
            }
            break;
        case eKeyBinding_DebugChunkBorders:
            if (m_f3Down)
            {
                QueueActionPress(eGameplayAction_ToggleChunkBorders);
                m_f3ComboUsed = true;
            }
            break;
        case eKeyBinding_DebugAdvancedTooltips:
            if (m_f3Down)
            {
                QueueActionPress(eGameplayAction_ToggleAdvancedTooltips);
                m_f3ComboUsed = true;
            }
            break;
        case eKeyBinding_DebugReloadChunks:
            if (m_f3Down)
            {
                QueueActionPress(eGameplayAction_ReloadChunks);
                m_f3ComboUsed = true;
            }
            break;
        case eKeyBinding_Screenshot:
            if ((SDL_GetModState() & KMOD_CTRL) == 0 || m_bindings[eKeyBinding_Panorama] != m_bindings[eKeyBinding_Screenshot])
            {
                QueueActionPress(eGameplayAction_Screenshot);
            }
            break;
        case eKeyBinding_Panorama:
            if ((SDL_GetModState() & KMOD_CTRL) != 0)
            {
                QueueActionPress(eGameplayAction_Panorama);
            }
            break;
        case eKeyBinding_HideHud:
            QueueActionPress(eGameplayAction_ToggleHud);
            break;
        default:
            break;
    }
}

void InputManager::DeactivateBinding(size_t binding)
{
    switch (binding)
    {
        case eKeyBinding_Forward:
            m_keyboardMoveForward = false;
            break;
        case eKeyBinding_Back:
            m_keyboardMoveBackward = false;
            break;
        case eKeyBinding_Left:
            m_keyboardMoveLeft = false;
            break;
        case eKeyBinding_Right:
            m_keyboardMoveRight = false;
            break;
        case eKeyBinding_Jump:
            m_keyboardJump = false;
            break;
        case eKeyBinding_Sneak:
            m_keyboardSneak     = m_keyboardToggleSneak && m_keyboardSneak;
            m_keyboardSneakHeld = false;
            ClearSneakToggle();
            break;
        case eKeyBinding_Sprint:
            m_keyboardSprint = m_keyboardToggleSprint && m_keyboardSprint;
            break;
        case eKeyBinding_PlayerList:
            m_keyboardPlayerList = false;
            break;
        case eKeyBinding_Attack:
            m_keyboardAttack = false;
            break;
        case eKeyBinding_Use:
            m_keyboardUse = false;
            break;
        case eKeyBinding_DebugMenu:
            if (m_f3Down && !m_f3ComboUsed)
            {
                QueueActionPress((SDL_GetModState() & KMOD_SHIFT) != 0 ? eGameplayAction_ToggleProfiler : eGameplayAction_ToggleDebugOverlay);
            }

            m_f3Down      = false;
            m_f3ComboUsed = false;
            break;
        default:
            break;
    }
}

std::wstring InputManager::GetInputCodeName(int inputCode)
{
    if (inputCode >= s_mouseButtonInputBase)
    {
        int button = inputCode - s_mouseButtonInputBase;
        switch (button)
        {
            case SDL_BUTTON_LEFT:
                return Everstead::GetInstance()->GetString("key.mouse.left");
            case SDL_BUTTON_RIGHT:
                return Everstead::GetInstance()->GetString("key.mouse.right");
            case SDL_BUTTON_MIDDLE:
                return Everstead::GetInstance()->GetString("key.mouse.middle");
            default:
                return Everstead::GetInstance()->GetString("key.mouse") + L" " + std::to_wstring(button);
        }
    }

    const char *name = SDL_GetScancodeName((SDL_Scancode) inputCode);
    if (name && name[0] != '\0')
    {
        std::string scancodeName(name);
        return std::wstring(scancodeName.begin(), scancodeName.end());
    }

    return Everstead::GetInstance()->GetString("key.none");
}

void InputManager::CaptureMoveSample()
{
    float moveForward = GetMoveForward();
    float moveStrafe  = GetMoveStrafe();
    if (fabsf(moveForward) >= 0.0001f || fabsf(moveStrafe) >= 0.0001f)
    {
        m_pendingMoveForward = moveForward;
        m_pendingMoveStrafe  = moveStrafe;
    }
}

#endif
