#pragma once

#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_MainMenu : public UIScene
{
public:
    UIScene_MainMenu();
    ~UIScene_MainMenu() override;

    void OnEnter() override;
    void Tick() override;

private:
    static void HandleButtonPressed(void *context, uint32_t id);
    void RefreshButtons();

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_multiList;
    UIControl_Button *m_serversButton;
    bool m_lastLoggedIn;
};
