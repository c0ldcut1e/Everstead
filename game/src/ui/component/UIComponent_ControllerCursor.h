#pragma once

#include "UIComponent.h"

class UIComponent_ControllerCursor : public UIComponent
{
public:
    UIComponent_ControllerCursor();
    ~UIComponent_ControllerCursor() override;

    void Render() override;
    void SetPosition(float actualX, float actualY);
    void SetSize(float actualSize);
    float GetSize() const;

private:
    float m_x;
    float m_y;
    float m_size;
};
