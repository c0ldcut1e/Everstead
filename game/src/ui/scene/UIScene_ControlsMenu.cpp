#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_ControlsMenu.h"

#include "../../core/Everstead.h"
#include "../Screen.h"

UIScene_ControlsMenu::UIScene_ControlsMenu() : UIScene(eUIScene_ControlsMenu)
{
    m_groupRoot      = new UIGroup("Group_ControlsMenu");
    m_backgroundFill = new UIComponent_Fill(0x70000000);
    m_multiList      = {new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f)};
    m_multiList->SetPanelVisible(false);

    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(15.0f);

    Everstead *everstead = Everstead::GetInstance();
    m_multiList->AddNewButton(s_controlsControllerId, everstead->GetString("menu.controls.controller"));
    m_multiList->AddNewButton(s_controlsKeyboardMouseId, everstead->GetString("menu.controls.keyboard_mouse"));

    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);

    m_backgroundFill->SetVisible(false);
    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_ControlsMenu::~UIScene_ControlsMenu()
{}

void UIScene_ControlsMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
    m_multiList->ResetPointerInputState();
}

void UIScene_ControlsMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_ControlsMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    (void) context;

    if (id == s_controlsControllerId)
    {
        Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_ControllerMenu);
    }

    if (id == s_controlsKeyboardMouseId)
    {
        Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_KeyboardMouseMenu);
    }
}

#endif
