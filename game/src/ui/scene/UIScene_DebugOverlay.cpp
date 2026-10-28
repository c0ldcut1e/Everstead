#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_DebugOverlay.h"

#include "../component/UIComponent_DebugMenu.h"

UIScene_DebugOverlay::UIScene_DebugOverlay() : UIScene(eUIScene_DebugOverlay)
{
    m_groupRoot = new UIGroup("Group_DebugOverlay");
    m_debugMenu = new UIComponent_DebugMenu();
    m_groupRoot->AddComponent(m_debugMenu);
    AddGroup(m_groupRoot);
}

UIScene_DebugOverlay::~UIScene_DebugOverlay()
{}

void UIScene_DebugOverlay::SetProfilerVisible(bool profilerVisible)
{
    m_debugMenu->SetProfilerVisible(profilerVisible);
}

bool UIScene_DebugOverlay::IsProfilerVisible() const
{
    return m_debugMenu->IsProfilerVisible();
}

#endif
