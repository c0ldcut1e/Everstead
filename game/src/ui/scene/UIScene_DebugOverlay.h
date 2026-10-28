#pragma once

#include "UIScene.h"

class UIComponent_DebugMenu;

class UIScene_DebugOverlay : public UIScene
{
public:
    UIScene_DebugOverlay();
    ~UIScene_DebugOverlay() override;

    void SetProfilerVisible(bool profilerVisible);
    bool IsProfilerVisible() const;

private:
    UIGroup *m_groupRoot;
    UIComponent_DebugMenu *m_debugMenu;
};
