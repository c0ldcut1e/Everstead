#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_LanguagesMenu : public UIScene
{
public:
    UIScene_LanguagesMenu();
    ~UIScene_LanguagesMenu() override;

    void OnEnter() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_languagesMenuBackId     = 1;
    static const uint32_t s_languagesMenuLanguageId = 100;
    static void HandleButtonPressed(void *context, uint32_t id);

    void BuildLanguageList();
    void LayoutControls();

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    std::vector<std::string> m_languageCodes;
};
