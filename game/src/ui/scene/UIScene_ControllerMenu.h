#pragma once

#include <cstdint>
#include <string>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_ControllerMenu : public UIScene
{
public:
    UIScene_ControllerMenu();
    ~UIScene_ControllerMenu() override;

    void OnEnter() override;
    void Tick() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_controllerResetId        = 1;
    static const uint32_t s_controllerInvertLookId   = 2;
    static const uint32_t s_controllerSensitivityId  = 3;
    static const uint32_t s_controllerToggleSprintId = 4;
    static const uint32_t s_controllerToggleSneakId  = 5;
    static const uint32_t s_menuCursorSensitivityId  = 6;
    static const uint32_t s_controllerBindingBase    = 100;
    static const uint32_t s_bindingKeyColor          = 0xFFB0B0B0;
    static const int s_bindingSelectionOffset        = 6;

    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleCheckboxUpdated(void *context, uint32_t id, bool checked);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);
    static std::wstring FormatSensitivityText(const char *nameKey, float value);

    void BuildList();
    void Reposition();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    int m_rebindingControl;
};
