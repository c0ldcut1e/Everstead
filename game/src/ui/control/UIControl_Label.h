#pragma once

#include <string>
#include <vector>

#include "UIControl.h"

class UIControl_Label : public UIControl
{
public:
    enum eWRAP_MODE : uint8_t
    {
        eWrapMode_None      = 0,
        eWrapMode_Word      = 1,
        eWrapMode_Character = 2,
    };

    enum eHORIZONTAL_ALIGN : uint8_t
    {
        eHorizontalAlign_Left   = 0,
        eHorizontalAlign_Center = 1,
        eHorizontalAlign_Right  = 2,
    };

    UIControl_Label(float x, float y, float width, float height, const std::wstring &text,
                    uint32_t color = 0xFF717171, bool shadow = false);
    ~UIControl_Label() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);
    bool IsSelectable() const override;
    void ResizeHorizontal(float width) override;
    void ResizeVertical(float height) override;

    void SetText(const std::wstring &text);
    const std::wstring &GetText() const;

    void SetColor(uint32_t color);
    void SetShadow(bool shadow);
    void SetShadowColor(uint32_t shadowColor);
    void SetOutline(bool outline);
    bool HasOutline() const;

    void SetTextScale(float textScale);
    float GetTextScale() const;

    void SetHorizontalAlign(eHORIZONTAL_ALIGN align);
    eHORIZONTAL_ALIGN GetHorizontalAlign() const;

    void SetWrapMode(eWRAP_MODE wrapMode);
    eWRAP_MODE GetWrapMode() const;

    void SetWrapWidth(float wrapWidth);
    float GetWrapWidth() const;

    void SetLineSpacing(float lineSpacing);
    float GetLineSpacing() const;

private:
    static const float s_labelTextScale;
    static const float s_labelLineSpacing;
    float MeasureTextWidth() const;
    float GetEffectiveWrapWidth() const;
    std::vector<std::wstring> ComputeWrappedLines() const;

    std::wstring m_text;
    uint32_t m_color;
    bool m_shadow;
    uint32_t m_shadowColor;
    bool m_outline;
    float m_textScale;
    eHORIZONTAL_ALIGN m_horizontalAlign;
    float m_preferredWidth;
    float m_preferredHeight;
    eWRAP_MODE m_wrapMode;
    float m_wrapWidth;
    float m_lineSpacing;
};
