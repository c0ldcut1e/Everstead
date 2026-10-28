#pragma once

#include <string>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_NetworkMenu : public UIScene
{
public:
    UIScene_NetworkMenu();
    ~UIScene_NetworkMenu() override;

    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_networkMenuStatusId        = 1;
    static const uint32_t s_networkMenuUsernameId      = 2;
    static const uint32_t s_networkMenuPasswordId      = 3;
    static const uint32_t s_networkMenuLoginId         = 4;
    static const uint32_t s_networkMenuLogoutId        = 5;
    static const uint32_t s_networkMenuUsernameLabelId = 7;
    static const uint32_t s_networkMenuPasswordLabelId = 8;
    static const uint32_t s_networkMenuFriendsId       = 9;
    static const uint32_t s_networkMenuInvitesId       = 10;
    static const uint32_t s_networkMenuBackId          = 11;
    static void HandleButtonPressed(void *context, uint32_t id);

    void RefreshView();
    void Login();
    void Logout();
    void ResizeMenu();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    std::wstring m_statusText;
};
