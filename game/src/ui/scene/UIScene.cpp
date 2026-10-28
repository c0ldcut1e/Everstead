#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene.h"

#include <algorithm>

#include "../../core/Everstead.h"
#include "../control/UIController.h"

void UIScene::PlayFocusSound(void *, uint32_t)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.focus");
}

UIScene::UIScene(eUISCENE id)
{
    m_id      = id;
    m_groups  = {};
    m_visible = true;
}

UIScene::~UIScene()
{
    for (UIGroup *group : m_groups)
    {
        if (group)
        {
            delete group;
        }
    }

    m_groups.clear();
}

void UIScene::TickInput()
{
    if (!m_visible)
    {
        return;
    }

    for (UIGroup *group : m_groups)
    {
        if (group)
        {
            group->TickInput();
        }
    }
}

void UIScene::Tick()
{
    if (!m_visible)
    {
        return;
    }

    for (UIGroup *group : m_groups)
    {
        if (group)
        {
            group->Tick();
        }
    }
}

void UIScene::Render()
{
    if (!m_visible)
    {
        return;
    }

    for (UIGroup *group : m_groups)
    {
        if (group)
        {
            group->Render();
        }
    }
}

void UIScene::UpdateStackStates(int sceneStackIndex, int topSceneStackIndex)
{
    int topGroupStackIndex = (int) m_groups.size() - 1;
    for (size_t i = 0; i < m_groups.size(); i++)
    {
        UIGroup *group = m_groups[i];
        if (!group)
        {
            continue;
        }

        bool enabled = m_visible;
        group->SetTickEnabled(enabled);
        group->SetInputEnabled(enabled && sceneStackIndex == topSceneStackIndex && (int) i == topGroupStackIndex);
        group->SetRenderEnabled(enabled);
    }
}

void UIScene::OnEnter()
{}

void UIScene::OnExit()
{}

void UIScene::OnGameplayAction(eGAMEPLAY_ACTION)
{}

void UIScene::AddGroup(UIGroup *group)
{
    if (!group)
    {
        return;
    }

    m_groups.push_back(group);
}

void UIScene::RemoveGroup(UIGroup *group)
{
    if (!group)
    {
        return;
    }

    m_groups.erase(std::remove(m_groups.begin(), m_groups.end(), group), m_groups.end());
}

void UIScene::ToggleVisible()
{
    m_visible = !m_visible;
}

void UIScene::SetVisible(bool visible)
{
    m_visible = visible;
}

bool UIScene::IsVisible() const
{
    return m_visible;
}

eUISCENE UIScene::GetId() const
{
    return m_id;
}

#endif
