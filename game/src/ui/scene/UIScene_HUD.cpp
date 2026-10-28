#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_HUD.h"

#include "../../core/GameInfo.h"
#include "../component/UIComponent_Chat.h"
#include "../component/UIComponent_Crosshair.h"
#include "../component/UIComponent_FpsCounter.h"
#include "../component/UIComponent_Hotbar.h"
#include "../component/UIComponent_PlayerList.h"
#include "../component/UIComponent_StatusBars.h"
#include "../component/UIComponent_StickFightScores.h"
#include "../component/UIComponent_Testing.h"
#include "../component/UIComponent_Vignette.h"
#include "../component/UIComponent_WaterOverlay.h"

UIScene_HUD::UIScene_HUD() : UIScene(eUIScene_HUD)
{
    m_groupRoot = new UIGroup("Group_HUD");
    m_chat      = new UIComponent_Chat();
    m_crosshair = new UIComponent_Crosshair();
    m_hotbar    = new UIComponent_Hotbar();
    m_groupRoot->AddComponent(new UIComponent_WaterOverlay());
    m_groupRoot->AddComponent(new UIComponent_Vignette());
    m_groupRoot->AddComponent(m_chat);
    m_groupRoot->AddComponent(m_crosshair);
    m_groupRoot->AddComponent(m_hotbar);
    m_groupRoot->AddComponent(new UIComponent_StatusBars());
    m_groupRoot->AddComponent(new UIComponent_PlayerList());
    m_groupRoot->AddComponent(new UIComponent_FpsCounter());
    m_groupRoot->AddComponent(new UIComponent_Testing);
    if (kStickFight)
    {
        m_groupRoot->AddComponent(new UIComponent_StickFightScores());
    }

    AddGroup(m_groupRoot);
}

UIScene_HUD::~UIScene_HUD()
{}

#endif
