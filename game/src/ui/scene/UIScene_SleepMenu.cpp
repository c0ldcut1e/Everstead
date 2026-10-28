#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_SleepMenu.h"

#include "../../core/Everstead.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../input/InputManager.h"
#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/PlayerActionC2SPacket.h"
#include "../Screen.h"

UIScene_SleepMenu::UIScene_SleepMenu() : UIScene(eUIScene_SleepMenu)
{
    Everstead *everstead = Everstead::GetInstance();
    m_groupRoot          = new UIGroup("Group_SleepMenu");
    m_backgroundFill     = new UIComponent_Fill(0x00101020);

    m_multiList = new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f);
    m_multiList->SetPanelVisible(false);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);
    m_multiList->AddNewButton(0, everstead->GetString("multiplayer.stopSleeping"));
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, Screen::HEIGHT - m_multiList->GetHeight() - 120.0f);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);
}

UIScene_SleepMenu::~UIScene_SleepMenu()
{}

void UIScene_SleepMenu::Tick()
{
    UIScene::Tick();
    m_multiList->SetPointerOnly(!Everstead::GetInstance()->GetInputManager()->IsUsingController());
    uint32_t alpha = (uint32_t) (220 * Everstead::GetInstance()->GetLocalPlayer()->GetSleepTimer() / 100);
    m_backgroundFill->SetColor(alpha << 24 | 0x101020);
}

void UIScene_SleepMenu::OnEnter()
{
    m_multiList->SetVisible(true);
    m_multiList->ResetPointerInputState();
}

void UIScene_SleepMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action == eGameplayAction_Back)
    {
        HandleButtonPressed(this, 0);
    }
}

void UIScene_SleepMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->GetUiController()->PlayUiSound("ui.press");
    (void) context;
    (void) id;
    everstead->GetLocalPlayer()->GetClientConnection()->Send(
            std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_StopSleeping, BlockPos(0, 0, 0), 0, 0)));
}

#endif
