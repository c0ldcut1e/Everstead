#pragma once

#include <string>
#include <vector>

#include "component/UIComponent.h"

class UIGroup
{
public:
    explicit UIGroup(const std::string &name);
    ~UIGroup();

    void Tick();
    void TickInput();
    void Render();

    void AddComponent(UIComponent *component);
    void RemoveComponent(UIComponent *component);

    void SetTickEnabled(bool enabled);
    bool IsTickEnabled() const;

    void SetInputEnabled(bool enabled);
    bool IsInputEnabled() const;

    void SetRenderEnabled(bool enabled);
    bool IsRenderEnabled() const;

    const std::string &GetName() const;

private:
    std::string m_name;
    std::vector<UIComponent *> m_components;
    bool m_tickEnabled;
    bool m_inputEnabled;
    bool m_renderEnabled;
};
