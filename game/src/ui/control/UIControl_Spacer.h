#pragma once

#include "UIControl.h"

class UIControl_Spacer : public UIControl
{
public:
    UIControl_Spacer(float x, float y, float width, float height);
    ~UIControl_Spacer() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);

    bool IsSelectable() const override;

private:
    float m_preferredWidth;
    float m_preferredHeight;
};
