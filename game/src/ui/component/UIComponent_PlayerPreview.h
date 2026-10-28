#pragma once

#include "UIComponent.h"

class UIComponent_PlayerPreview : public UIComponent
{
public:
    UIComponent_PlayerPreview();
    ~UIComponent_PlayerPreview() override;

    void Render() override;

    void SetBounds(float x, float y, float width, float height);
    void SetLookTarget(float actualX, float actualY);

private:
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    float m_lookX;
    float m_lookY;
};
