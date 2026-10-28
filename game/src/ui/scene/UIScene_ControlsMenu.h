#pragma once

#include <cstdint>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_ControlsMenu : public UIScene
{
public:
    UIScene_ControlsMenu();
    ~UIScene_ControlsMenu() override;

    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_controlsControllerId    = 1;
    static const uint32_t s_controlsKeyboardMouseId = 2;

    static void HandleButtonPressed(void *context, uint32_t id);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
};
