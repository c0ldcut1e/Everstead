#pragma once

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_PauseMenu : public UIScene
{
public:
    UIScene_PauseMenu();
    ~UIScene_PauseMenu() override;

    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    enum eEXIT_BUTTON : uint32_t
    {
        eExitButton_Cancel = 0,
        eExitButton_Save,
        eExitButton_NoSave
    };

    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleExitButtonPressed(void *context, uint32_t id);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    UIControl_Button *m_saveButton;
};
