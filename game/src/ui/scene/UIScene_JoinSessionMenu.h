#pragma once

#include <cstdint>

#include "../../network/MultiplayerSession.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_JoinSessionMenu : public UIScene
{
public:
    UIScene_JoinSessionMenu();
    ~UIScene_JoinSessionMenu() override;

    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const float s_joinSessionPanelWidth;
    static const float s_joinSessionPanelPaddingX;
    static const float s_joinSessionPanelPaddingY;
    static const float s_joinSessionButtonWidth;
    static const float s_joinSessionJoinButtonHeight;
    static const float s_joinSessionPlayerRowHeight;
    static const float s_joinSessionRowSpacing;
    static const float s_joinSessionJoinButtonGap;
    static const float s_joinSessionScrollIconTopGap;
    static const float s_joinSessionScrollIconVirtualHeight;
    static const float s_joinSessionScrollIconBottomPadding;
    static const int s_joinSessionVisibleRows       = 6;
    static const int s_joinSessionVisiblePlayers    = 4;
    static const uint32_t s_joinSessionJoinId       = 1;
    static const uint32_t s_joinSessionSpacerId     = 2;
    static const uint32_t s_joinSessionPlayerBaseId = 1000;
    static void HandleButtonPressed(void *context, uint32_t id);

    void BuildPanel();
    void LayoutControls();
    std::wstring BuildPlayerLabel(const MultiplayerSession::Player &player) const;

    UIGroup *m_groupRoot;
    UIControl_MultiList *m_panel;
    MultiplayerSession::SessionInfo m_session;
};
