#pragma once

#include <cstdint>
#include <string>

#include "../../input/ControllerGlyphs.h"
#include "../UITextureAtlas.h"
#include "UIControl.h"

class BufferBuilder;

class UIControl_Button : public UIControl
{
public:
    enum eHORIZONTAL_ALIGN : uint8_t
    {
        eHorizontalAlign_Left   = 0,
        eHorizontalAlign_Center = 1,
    };

    UIControl_Button(float x, float y, float width, float height, const std::wstring &label);
    UIControl_Button(float x, float y, float width, float height, const std::wstring &label, const char *normalTexturePath,
                     const char *highlightTexturePath);
    ~UIControl_Button() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);
    void SetTexelLength(float texelLength);

    void Activate() override;
    bool OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight) override;

    const std::wstring &GetLabel() const;
    void SetLabel(const std::wstring &label);
    void SetHorizontalAlign(eHORIZONTAL_ALIGN align);
    eHORIZONTAL_ALIGN GetHorizontalAlign() const;
    void SetRightLabel(const std::wstring &rightLabel, uint32_t color);
    void SetRightIcon(eCONTROLLER_BUTTON button);

protected:
    UIControl_Button(float x, float y, float width, float height, const std::wstring &label, const char *normalTexturePath,
                     const char *highlightTexturePath, float preferredHeight, float preferredWidth = 0.0f);

private:
    static constexpr float kRightIconScale = 0.55f;
    static constexpr float kBorderTexels   = 6.0f;

    static const float s_buttonDefaultWidth;
    static const float s_buttonDefaultHeight;
    static const float s_buttonTextScale;
    static const uint32_t s_buttonSelectedTextColor = 0xFFFFFF00;
    static const uint32_t s_buttonDefaultTextColor  = 0xFFFFFFFF;
    void EmitButtonQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, uint32_t color,
                        const UITextureAtlas::Region &region);

    std::wstring m_label;
    std::wstring m_rightLabel;
    uint32_t m_rightLabelColor;
    eCONTROLLER_BUTTON m_rightIcon;
    UITextureAtlas::Region m_normalRegion;
    UITextureAtlas::Region m_highlightRegion;
    float m_preferredHeight;
    float m_preferredWidth;
    float m_texelLength;
    eHORIZONTAL_ALIGN m_horizontalAlign;
    bool m_wasPressed;
    uint32_t m_pressedAtMs;
};
