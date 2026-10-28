#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../network/MultiplayerSession.h"
#include "../../world/save/WorldSave.h"
#include "../UITextureAtlas.h"
#include "../control/UIControl_Label.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_LoadCreateJoin : public UIScene
{
public:
    UIScene_LoadCreateJoin();
    ~UIScene_LoadCreateJoin() override;

    void OnEnter() override;
    void TickInput() override;
    void Tick() override;
    void Render() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const float s_createLoadJoinMenuWidth;
    static const float s_createLoadJoinMenuHeight;
    static const float s_createLoadJoinTabOffsetY;
    static const float s_createLoadJoinTabTextScale;
    static const float s_createLoadJoinTabTextRaise;
    static const float s_createLoadJoinTabTextY;
    static const float s_createLoadJoinTabTextHeight;
    static const float s_createLoadJoinListX;
    static const float s_createLoadJoinListY;
    static const float s_createLoadJoinListButtonWidth;
    static const float s_createLoadJoinListButtonHeight;
    static const int s_createLoadJoinListVisibleRows               = 6;
    static const uint32_t s_createLoadJoinCreateId                 = 50000;
    static const uint32_t s_createLoadJoinSessionBaseId            = 60000;
    static const uint32_t s_createLoadJoinWorldPanelRenameInputId  = 70000;
    static const uint32_t s_createLoadJoinWorldPanelRenameId       = 70001;
    static const uint32_t s_createLoadJoinWorldPanelDuplicateId    = 70002;
    static const uint32_t s_createLoadJoinWorldPanelDeleteId       = 70003;
    static const uint32_t s_createLoadJoinWorldPanelBackId         = 70004;
    static const uint32_t s_createLoadJoinSessionRefreshIntervalMs = 5000;
    static const uint32_t s_createLoadJoinTabTextColor             = 0xFF717171;
    static void HandleListButtonPressed(void *context, uint32_t id);
    static void HandleWorldPanelButtonPressed(void *context, uint32_t id);

    void BuildTabContent();
    void RefreshJoinSessions();
    void RequestSessionRefresh();
    void SetSelectedTab(int selectedTab);
    void LayoutControls();
    void HandleTabMouseInput();
    void HandleWorldPanelMouseInput();
    void ShowWorldPanel(size_t worldIndex);
    void HideWorldPanel();
    void RenamePanelWorld();
    void DuplicatePanelWorld();
    void DeletePanelWorld();
    void DrawMenuBackground();
    void DrawRegion(const UITextureAtlas::Region &region, float x0, float y0, float x1, float y1);

    UIGroup *m_groupRoot;
    UIControl_Label *m_tabLabels[3];
    UIControl_MultiList *m_list;
    UIControl_MultiList *m_worldPanel;
    UITextureAtlas::Region m_backgroundRegion;
    UITextureAtlas::Region m_tabRegions[3];
    std::vector<WorldInfo> m_worlds;
    std::vector<MultiplayerSession::SessionInfo> m_sessions;
    size_t m_worldPanelIndex;
    int m_selectedTab;
    uint32_t m_lastJoinSessionRefreshMs;
    uint32_t m_lastSessionsVersion;
    bool m_previousMousePrimaryDown;
    bool m_previousMouseSecondaryDown;
    bool m_wasVisible;
};
