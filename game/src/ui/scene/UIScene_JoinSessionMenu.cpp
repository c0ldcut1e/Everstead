#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_JoinSessionMenu.h"

#include <string>

#include "../../core/Everstead.h"
#include "../../network/MultiplayerSession.h"
#include "../../utils/StringUtils.h"
#include "../Screen.h"

const float UIScene_JoinSessionMenu::s_joinSessionPanelWidth              = 560.0f;
const float UIScene_JoinSessionMenu::s_joinSessionPanelPaddingX           = 18.0f;
const float UIScene_JoinSessionMenu::s_joinSessionPanelPaddingY           = 16.0f;
const float UIScene_JoinSessionMenu::s_joinSessionButtonWidth             = 520.0f;
const float UIScene_JoinSessionMenu::s_joinSessionJoinButtonHeight        = 58.0f;
const float UIScene_JoinSessionMenu::s_joinSessionPlayerRowHeight         = 80.0f;
const float UIScene_JoinSessionMenu::s_joinSessionRowSpacing              = 8.0f;
const float UIScene_JoinSessionMenu::s_joinSessionJoinButtonGap           = 18.0f;
const float UIScene_JoinSessionMenu::s_joinSessionScrollIconTopGap        = 8.0f;
const float UIScene_JoinSessionMenu::s_joinSessionScrollIconVirtualHeight = 25.0f;
const float UIScene_JoinSessionMenu::s_joinSessionScrollIconBottomPadding = 20.0f;

UIScene_JoinSessionMenu::UIScene_JoinSessionMenu() : UIScene(eUIScene_JoinSessionMenu)
{
    m_groupRoot = new UIGroup("Group_JoinSessionMenu");
    m_session   = Everstead::GetInstance()->GetMultiplayerSession()->GetSelectedFriendSession();
    m_panel     = new UIControl_MultiList(0.0f, 0.0f, s_joinSessionPanelWidth, 0.0f);
    m_panel->SetContentPadding(s_joinSessionPanelPaddingX, 0.0f);
    m_panel->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_panel->SetButtonRowWidth(s_joinSessionButtonWidth);
    m_panel->SetButtonRowHeight(s_joinSessionJoinButtonHeight);
    m_panel->SetRowSpacing(s_joinSessionRowSpacing);
    m_panel->SetMaxVisibleElements(s_joinSessionVisibleRows);
    m_panel->SetFixedTopElements(2);
    m_panel->SetButtonPressedCallback(HandleButtonPressed, this);
    m_panel->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    BuildPanel();
    LayoutControls();

    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_JoinSessionMenu::~UIScene_JoinSessionMenu()
{}

void UIScene_JoinSessionMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    Everstead::GetInstance()->GetUiController()->CloseScene();
}

void UIScene_JoinSessionMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    if (id != s_joinSessionJoinId)
    {
        return;
    }

    UIScene_JoinSessionMenu *scene = (UIScene_JoinSessionMenu *) context;
    Everstead::GetInstance()->StartJoinedWorld(scene->m_session.sid, scene->m_session.name);
}

void UIScene_JoinSessionMenu::BuildPanel()
{
    m_panel->ClearEntries();
    UIControl_Button *joinButton = m_panel->AddNewButton(s_joinSessionJoinId, Everstead::GetInstance()->GetString("world.join.join_game"));
    joinButton->SetPreferredHeight(s_joinSessionJoinButtonHeight);
    m_panel->AddNewSpacer(s_joinSessionSpacerId, s_joinSessionJoinButtonGap + s_joinSessionPanelPaddingY - s_joinSessionRowSpacing * 2.0f);

    const std::vector<MultiplayerSession::Player> &players = m_session.players;
    if (players.empty())
    {
        UIControl_Button *button = m_panel->AddNewButton(s_joinSessionPlayerBaseId, BuildPlayerLabel(MultiplayerSession::Player()));
        button->SetPreferredHeight(s_joinSessionPlayerRowHeight);
        button->SetHorizontalAlign(UIControl_Button::eHorizontalAlign_Left);
    }
    else
    {
        for (size_t i = 0; i < players.size(); i++)
        {
            UIControl_Button *button = m_panel->AddNewButton(s_joinSessionPlayerBaseId + (uint32_t) i, BuildPlayerLabel(players[i]));
            button->SetPreferredHeight(s_joinSessionPlayerRowHeight);
            button->SetHorizontalAlign(UIControl_Button::eHorizontalAlign_Left);
        }
    }

    m_panel->ResizeHorizontal();
    m_panel->SetSize(s_joinSessionPanelWidth, s_joinSessionJoinButtonHeight + s_joinSessionJoinButtonGap + s_joinSessionPanelPaddingY * 2.0f +
                                                      s_joinSessionPlayerRowHeight * (float) s_joinSessionVisiblePlayers +
                                                      s_joinSessionRowSpacing * (float) (s_joinSessionVisiblePlayers - 1) +
                                                      s_joinSessionScrollIconTopGap + s_joinSessionScrollIconVirtualHeight +
                                                      s_joinSessionScrollIconBottomPadding);
    m_panel->SetPanelBoundsOffset(0.0f, s_joinSessionJoinButtonHeight + s_joinSessionJoinButtonGap, s_joinSessionPanelWidth,
                                  s_joinSessionPanelPaddingY * 2.0f + s_joinSessionPlayerRowHeight * (float) s_joinSessionVisiblePlayers +
                                          s_joinSessionRowSpacing * (float) (s_joinSessionVisiblePlayers - 1) + s_joinSessionScrollIconTopGap +
                                          s_joinSessionScrollIconVirtualHeight + s_joinSessionScrollIconBottomPadding);
}

void UIScene_JoinSessionMenu::LayoutControls()
{
    float panelX = (Screen::WIDTH - m_panel->GetWidth()) * 0.5f;
    float listY  = (Screen::HEIGHT - m_panel->GetHeight()) * 0.5f;

    m_panel->SetPosition(panelX, listY);
}

std::wstring UIScene_JoinSessionMenu::BuildPlayerLabel(const MultiplayerSession::Player &player) const
{
    std::wstring label =
            player.username.empty() ? Everstead::GetInstance()->GetString("friends.unknown_player") : StringUtils::ToUtf16(player.username);
    if (player.host)
    {
        label += L" (" + Everstead::GetInstance()->GetString("world.join.host") + L")";
    }

    return label;
}

#endif
