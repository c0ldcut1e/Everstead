#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_LanguagesMenu.h"

#include "../../core/Everstead.h"
#include "../Screen.h"

UIScene_LanguagesMenu::UIScene_LanguagesMenu() : UIScene(eUIScene_LanguagesMenu)
{
    m_groupRoot      = new UIGroup("Group_LanguagesMenu");
    m_backgroundFill = new UIComponent_Fill(0x70000000);
    m_multiList      = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_languageCodes  = {};

    m_backgroundFill->SetVisible(false);

    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetRowSpacing(15.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_LanguagesMenu::~UIScene_LanguagesMenu()
{}

void UIScene_LanguagesMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    BuildLanguageList();
    m_multiList->ResetPointerInputState();
}

void UIScene_LanguagesMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_LanguagesMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_LanguagesMenu *_this = (UIScene_LanguagesMenu *) context;
    if (id == s_languagesMenuBackId)
    {
        Everstead::GetInstance()->GetUiController()->CloseScene();
        return;
    }

    if (id < s_languagesMenuLanguageId)
    {
        return;
    }

    uint32_t index = id - s_languagesMenuLanguageId;
    if (index >= _this->m_languageCodes.size())
    {
        return;
    }

    Localization *localization = Localization::GetInstance();
    if (localization->LoadLanguage(_this->m_languageCodes[index]))
    {
        Everstead::GetInstance()->options.language = (int8_t) localization->GetLanguageIndex(localization->GetLanguageCode());
        _this->BuildLanguageList();
    }
}

void UIScene_LanguagesMenu::BuildLanguageList()
{
    Localization *localization = Localization::GetInstance();
    localization->ReloadLanguages();

    m_multiList->ClearEntries();
    m_multiList->SetSize(0.0f, 0.0f);
    m_languageCodes.clear();

    const std::vector<Localization::LanguageInfo> &languages = localization->GetLanguages();
    for (size_t i = 0; i < languages.size(); i++)
    {
        std::wstring name = languages[i].name;
        if (languages[i].code == localization->GetLanguageCode())
        {
            name += localization->Get("menu.languages.selected_suffix");
        }

        m_multiList->AddNewButton(s_languagesMenuLanguageId + (uint32_t) i, name);
        m_languageCodes.push_back(languages[i].code);
    }

    m_multiList->AddNewButton(s_languagesMenuBackId, localization->Get("button.back"));
    LayoutControls();
}

void UIScene_LanguagesMenu::LayoutControls()
{
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

#endif
