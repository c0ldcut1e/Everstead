#pragma once

#include <string>

#include "../UITextureAtlas.h"
#include "UIControl.h"

class BufferBuilder;

class UIControl_Checkbox : public UIControl
{
public:
    UIControl_Checkbox(float x, float y, float width, float height, const std::wstring &label,
                       bool checked = false);
    ~UIControl_Checkbox() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);
    void ResizeHorizontal(float width) override;
    void ResizeVertical(float height) override;

    void Activate() override;
    bool OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight) override;

    void SetChecked(bool checked);
    bool IsChecked() const;

private:
    static const float s_checkboxVirtualSize;
    static const float s_checkboxLabelScale;
    static const float s_checkboxLabelSpacing;
    static const float s_checkboxLabelYOffset;
    static const float s_checkboxDisabledAlpha;
    void EmitCheckboxQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0,
                          float v0, float u1, float v1, uint32_t color,
                          const UITextureAtlas::Region &region);

    float MeasureLabelWidth() const;

    std::wstring m_label;
    bool m_checked;
    float m_preferredWidth;
    float m_preferredHeight;
    UITextureAtlas::Region m_backgroundRegion;
    UITextureAtlas::Region m_overRegion;
    UITextureAtlas::Region m_tickRegion;
};
