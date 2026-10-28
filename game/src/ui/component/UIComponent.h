#pragma once

class UIComponent
{
public:
    UIComponent();
    virtual ~UIComponent();

    virtual void Tick();
    virtual void Render();

    void ToggleVisible();
    void SetVisible(bool visible);
    bool IsVisible() const;

private:
    bool m_visible;
};
