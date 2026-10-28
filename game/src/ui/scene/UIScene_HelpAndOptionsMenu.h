#pragma once

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_HelpAndOptionsMenu : public UIScene
{
public:
    UIScene_HelpAndOptionsMenu();
    ~UIScene_HelpAndOptionsMenu() override;

    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_helpOptionsSettingsId   = 3;
    static const uint32_t s_helpOptionsChangeSkinId = 1;
    static const uint32_t s_helpOptionsControlsId   = 2;
    static const uint32_t s_helpOptionsCreditsId    = 4;
    static void HandleButtonPressed(void *context, uint32_t id);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
};
