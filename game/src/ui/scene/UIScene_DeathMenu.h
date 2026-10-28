#pragma once

#include <cstdint>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_Label.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_DeathMenu : public UIScene
{
public:
    UIScene_DeathMenu();
    ~UIScene_DeathMenu() override;

    void Tick() override;
    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static void HandleButtonPressed(void *context, uint32_t id);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_Label *m_titleLabel;
    UIControl_MultiList *m_multiList;
    UIControl_Button *m_respawnButton;
    UIControl_Button *m_exitButton;
    uint32_t m_enteredAtMs;
};
