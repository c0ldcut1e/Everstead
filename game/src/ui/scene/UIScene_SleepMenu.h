#pragma once

#include <cstdint>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_SleepMenu : public UIScene
{
public:
    UIScene_SleepMenu();
    ~UIScene_SleepMenu() override;

    void Tick() override;
    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static void HandleButtonPressed(void *context, uint32_t id);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
};
