#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "ControllerBackend.h"
#include "ControllerBinding.h"
#include "KeyBinding.h"

class InputManager
{
public:
    struct TextEditKey
    {
        int key;
        bool shift;
        bool control;
    };

    explicit InputManager(std::unique_ptr<ControllerBackend> controllerBackend);
    ~InputManager();

    void Update(double deltaTime);
    void Clear();

    void OnKeyPressed(int key);
    void OnKeyReleased(int key);
    void OnMouseButtonPressed(int button);
    void OnMouseButtonReleased(int button);
    void OnMouseMoved(double dx, double dy, bool look);
    void OnMouseWheel(int wheelY);
    void OnTextInput(const std::string &text);

    bool IsActionDown(eGAMEPLAY_ACTION action) const;
    bool IsUseItemHeld() const;
    bool IsDestroyHeld() const;
    bool IsSneakHeld() const;
    bool IsSneakToggled() const;
    void ClearSneakToggle();
    void SetGameplayInputBlocked(bool blocked);
    bool ConsumeActionPress(eGAMEPLAY_ACTION action);
    void ConsumeLocalPlayerTickInputs(float *moveForward, float *moveStrafe, bool *jumpHeld, bool *jumpPressed);

    float GetMoveForward() const;
    float GetMoveStrafe() const;
    int ConsumeForwardTapCount();
    int ConsumeHotbarScroll();
    int ConsumeHotbarSlot();
    void ConsumeLookDelta(double *dx, double *dy);
    bool ConsumeUiNavigateUpPress();
    bool ConsumeUiNavigateDownPress();
    bool ConsumeUiNavigateLeftPress();
    bool ConsumeUiNavigateRightPress();
    bool ConsumeUiActivatePress();
    bool ConsumeUiCancelPress();
    int ConsumeUiScrollDelta();
    void SetUiTextInputFocused(bool focused);
    bool IsUiTextInputFocused() const;
    std::string ConsumeUiTextInput();
    bool ConsumeUiTextBackspacePress();
    bool ConsumeUiTextDeletePress();
    bool ConsumeUiTextSubmitPress();
    bool ConsumeUiTextCancelPress();
    bool ConsumeUiTextCompletePress();
    int ConsumeUiTabDelta();
    void InjectUiTextSubmit();
    void SetUiTextReplacement(const std::wstring &text);
    bool ConsumeUiTextReplacement(std::wstring *text);
    void InjectUiTextCancel();

    bool IsUsingController() const;
    void GetControllerStick(float *x, float *y) const;
    bool IsControllerButtonPressed(eCONTROLLER_BUTTON button) const;
    bool IsControllerButtonDown(eCONTROLLER_BUTTON button) const;
    float GetControllerLookStickY() const;
    bool IsControllerButtonBound(eCONTROLLER_BUTTON button) const;
    eCONTROLLER_TYPE GetControllerType() const;
    void SetVirtualKeyboardActive(bool active);
    bool ConsumeControllerButtonPress(eCONTROLLER_BUTTON button);
    void ConsumeVirtualKeyboardNavigation(int *horizontal, int *vertical);
    void ConsumeUiTextEditKeys(std::vector<TextEditKey> *keys);
    void SetControllerLookSensitivity(double sensitivity);
    void SetToggleOptions(bool keyboardSprint, bool keyboardSneak, bool controllerSprint, bool controllerSneak);
    double GetControllerLookSensitivity() const;

    int GetBinding(eKEY_BINDING binding) const;
    void SetBinding(eKEY_BINDING binding, int inputCode);
    void ResetBindingsToDefault();
    void BeginRebind(eKEY_BINDING binding);
    void CancelRebind();
    bool IsRebinding() const;
    eKEY_BINDING GetRebindTarget() const;
    static int GetDefaultBinding(eKEY_BINDING binding);
    eCONTROLLER_BUTTON GetControllerBinding(eCONTROLLER_BINDING binding) const;
    void SetControllerBinding(eCONTROLLER_BINDING binding, eCONTROLLER_BUTTON button);
    void ResetControllerBindingsToDefault();
    void BeginControllerRebind(eCONTROLLER_BINDING binding);
    void CancelControllerRebind();
    bool IsControllerRebinding() const;
    static std::wstring GetInputCodeName(int inputCode);

private:
    static const size_t s_maxPendingTextEditKeys = 64;

    void CaptureMoveSample();
    void ApplyControllerBindings(const ControllerState &state);
    void QueueActionPress(eGAMEPLAY_ACTION action);
    void ActivateBinding(size_t binding);
    void DeactivateBinding(size_t binding);

    bool m_keyboardMoveForward;
    bool m_keyboardMoveBackward;
    bool m_keyboardMoveLeft;
    bool m_keyboardMoveRight;
    bool m_keyboardJump;
    bool m_keyboardAttack;
    bool m_keyboardUse;
    bool m_keyboardSprint;
    bool m_keyboardSneak;
    bool m_keyboardSneakHeld;
    bool m_keyboardToggleSprint;
    bool m_keyboardToggleSneak;
    bool m_keyboardPlayerList;
    bool m_f3Down;
    bool m_f3ComboUsed;

    float m_controllerMoveForward;
    float m_controllerMoveStrafe;
    float m_controllerLookStickY;
    float m_pendingMoveForward;
    float m_pendingMoveStrafe;
    int m_forwardTapCount;

    double m_lookDeltaX;
    double m_lookDeltaY;
    int m_pendingHotbarScroll;
    int m_pendingHotbarSlot;
    bool m_pendingUiNavigateUpPress;
    bool m_pendingUiNavigateDownPress;
    bool m_pendingUiNavigateLeftPress;
    bool m_pendingUiNavigateRightPress;
    bool m_pendingUiActivatePress;
    bool m_pendingUiCancelPress;
    int m_pendingUiScrollDelta;
    bool m_uiTextInputFocused;
    std::string m_pendingUiTextInput;
    bool m_pendingUiTextBackspacePress;
    bool m_pendingUiTextDeletePress;
    bool m_pendingUiTextSubmitPress;
    bool m_pendingUiTextCancelPress;
    bool m_pendingUiTextCompletePress;
    std::vector<TextEditKey> m_pendingUiTextEditKeys;
    std::wstring m_pendingUiTextReplacement;
    bool m_hasUiTextReplacement;
    bool m_usingController;
    eCONTROLLER_TYPE m_controllerType;
    bool m_virtualKeyboardActive;
    int m_uiTabDelta;
    std::array<bool, g_controllerButtonCount> m_controllerButtonsPressed;
    std::array<bool, g_controllerButtonCount> m_controllerButtonsDown;
    bool m_controllerSneakHeld;
    std::array<bool, g_controllerButtonCount> m_pendingControllerPresses;
    int m_pendingKeyboardHorizontal;
    int m_pendingKeyboardVertical;

    std::array<bool, g_gameplayActionCount> m_controllerActionDown;
    std::array<bool, g_gameplayActionCount> m_pendingActionPresses;

    std::array<int, g_keyBindingCount> m_bindings;
    int m_rebindTarget;
    int m_controllerRebindTarget;
    bool m_controllerSneakToggled;
    bool m_gameplayInputBlocked;
    bool m_controllerSprintToggled;
    bool m_controllerToggleSprint;
    bool m_controllerToggleSneak;
    std::array<eCONTROLLER_BUTTON, g_controllerBindingCount> m_controllerBindings;

    std::unique_ptr<ControllerBackend> m_controllerBackend;
};
