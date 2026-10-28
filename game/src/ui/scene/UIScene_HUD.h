#pragma once

#include "UIScene.h"

class UIScene_HUD : public UIScene
{
public:
    UIScene_HUD();
    ~UIScene_HUD() override;

private:
    UIGroup *m_groupRoot;
    UIComponent *m_chat;
    UIComponent *m_crosshair;
    UIComponent *m_hotbar;
};
