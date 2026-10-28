#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_DeathMenu.h"

#include "../../core/Everstead.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/RespawnC2SPacket.h"
#include "../../utils/Time.h"
#include "../Screen.h"

#define DEATH_MENU_RESPAWN_ID 1
#define DEATH_MENU_EXIT_ID    2

UIScene_DeathMenu::UIScene_DeathMenu() : UIScene(eUIScene_DeathMenu)
{
    Everstead *everstead = Everstead::GetInstance();
    m_groupRoot          = new UIGroup("Group_DeathMenu");
    m_backgroundFill     = new UIComponent_Fill(0x80602020);
    m_titleLabel         = new UIControl_Label(0.0f, 180.0f, Screen::WIDTH, 60.0f, everstead->GetString("menu.death.title"), 0xFFFFFFFF, false);
    m_titleLabel->SetTextScale(2.0f);
    m_titleLabel->SetOutline(true);
    m_titleLabel->SetHorizontalAlign(UIControl_Label::eHorizontalAlign_Center);

    m_multiList = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_multiList->SetPanelVisible(false);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->SetRowSpacing(15.0f);
    m_respawnButton = m_multiList->AddNewButton(DEATH_MENU_RESPAWN_ID, everstead->GetString("menu.death.respawn"));
    m_exitButton    = m_multiList->AddNewButton(DEATH_MENU_EXIT_ID, everstead->GetString("menu.main.exit_game"));
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_titleLabel);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
    m_enteredAtMs = 0;
}

UIScene_DeathMenu::~UIScene_DeathMenu()
{}

void UIScene_DeathMenu::Tick()
{
    UIScene::Tick();
    bool enabled = Time::GetMilliseconds() - m_enteredAtMs >= 1000;
    m_respawnButton->SetEnabled(enabled);
    m_exitButton->SetEnabled(enabled);
}

void UIScene_DeathMenu::OnEnter()
{
    m_enteredAtMs = Time::GetMilliseconds();
    m_respawnButton->SetEnabled(false);
    m_exitButton->SetEnabled(false);
    m_multiList->SetVisible(true);
    m_multiList->ResetPointerInputState();
}

void UIScene_DeathMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    (void) action;
}

void UIScene_DeathMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetUiController()->PlayUiSound("ui.press");
    (void) context;

    switch (id)
    {
        case DEATH_MENU_RESPAWN_ID: {
            ((MultiPlayerLocalPlayer *) everstead->GetLocalPlayer())->GetClientConnection()->Send(std::shared_ptr<Packet>(new RespawnC2SPacket()));
            break;
        }

        case DEATH_MENU_EXIT_ID: {
            everstead->RequestLeaveWorld(true);
            break;
        }

        default: {
            break;
        }
    }
}

#endif
