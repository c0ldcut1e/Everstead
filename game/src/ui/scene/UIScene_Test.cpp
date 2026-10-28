#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_Test.h"

#include <memory>

#include "../UIPanelProperties_TestMenu.h"
#include "../component/UIComponent_Panel.h"

UIScene_Test::UIScene_Test() : UIScene(eUIScene_Test)
{
    m_groupRoot = new UIGroup("Group_Test");
    m_panel     = new UIComponent_Panel(std::make_unique<UIPanelProperties_TestMenu>());
    m_groupRoot->AddComponent(m_panel);
    AddGroup(m_groupRoot);
}

UIScene_Test::~UIScene_Test()
{}

#endif
