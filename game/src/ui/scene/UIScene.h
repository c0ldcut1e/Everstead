#pragma once

#include <string>
#include <vector>

#include "../../input/GameplayAction.h"
#include "../UIGroup.h"
#include "UISceneId.h"

class UIScene
{
public:
    explicit UIScene(eUISCENE id);
    virtual ~UIScene();

    virtual void TickInput();
    virtual void Tick();
    virtual void Render();
    virtual void UpdateStackStates(int sceneStackIndex, int topSceneStackIndex);

    virtual void OnEnter();
    virtual void OnExit();
    virtual void OnGameplayAction(eGAMEPLAY_ACTION action);

    void AddGroup(UIGroup *group);
    void RemoveGroup(UIGroup *group);

    void ToggleVisible();
    void SetVisible(bool visible);
    bool IsVisible() const;

    eUISCENE GetId() const;

    static void PlayFocusSound(void *context, uint32_t id);

private:
    eUISCENE m_id;
    std::vector<UIGroup *> m_groups;
    bool m_visible;
};
