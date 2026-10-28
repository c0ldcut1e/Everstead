#pragma once

#include <cstdint>
#include <string>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_KeyboardMouseMenu : public UIScene
{
public:
    UIScene_KeyboardMouseMenu();
    ~UIScene_KeyboardMouseMenu() override;

    void OnEnter() override;
    void Tick() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_keyboardMouseResetId           = 1;
    static const uint32_t s_keyboardMouseInvertLookId      = 2;
    static const uint32_t s_keyboardMouseSensitivityId     = 3;
    static const uint32_t s_keyboardMouseToggleSprintId    = 4;
    static const uint32_t s_keyboardMouseToggleSneakId     = 5;
    static const uint32_t s_keyboardMouseBindingBase       = 100;
    static const uint32_t s_bindingKeyColor                = 0xFFB0B0B0;
    static const int s_keyboardMouseBindingSelectionOffset = 5;

    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleCheckboxUpdated(void *context, uint32_t id, bool checked);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);
    static std::wstring FormatSliderText(const std::wstring &name, float value);

    void BuildList();
    void Reposition();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    int m_rebindingControl;
};
