#pragma once

#include <cstdint>
#include <string>

#include "../UITextureAtlas.h"
#include "UIControl.h"

class BufferBuilder;

class UIControl_Slider : public UIControl
{
public:
    UIControl_Slider(float x, float y, float width, float height, const std::wstring &name,
                     float minValue, float maxValue, float defaultValue, float increment);
    ~UIControl_Slider() override;

    void Render() override;

    float GetPreferredWidth() const override;
    float GetPreferredHeight() const override;
    void SetPreferredWidth(float preferredWidth);
    void SetPreferredHeight(float preferredHeight);

    bool StepValue(int direction) override;
    bool OnPointerPressed(int actualX, int actualY, int actualWidth, int actualHeight) override;
    bool OnPointerMoved(int actualX, int actualY, int actualWidth, int actualHeight) override;
    bool OnPointerReleased(int actualX, int actualY, int actualWidth, int actualHeight) override;

    bool SetValue(float value);
    float GetValue() const;

    float GetMinimumValue() const;
    float GetMaximumValue() const;
    float GetDefaultValue() const;
    float GetIncrement() const;
    const std::wstring &GetName() const;

    void ResetToDefaultValue();

    void SetText(const std::wstring &text);
    const std::wstring &GetText() const;

private:
    static const float s_sliderDefaultWidth;
    static const float s_sliderDefaultHeight;
    static const float s_sliderTextScale;
    static const uint32_t s_sliderSelectedTextColor    = 0xFFFFFF00;
    static const uint32_t s_sliderDefaultTextColor     = 0xFFFFFFFF;
    static const uint32_t s_sliderSelectedOutlineColor = 0xFFFFFF00;
    static const float s_sliderSourceBorderPixels;
    void EmitSliderQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0,
                        float v0, float u1, float v1, uint32_t color);
    void EmitSliderTextureQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1,
                               float u0, float v0, float u1, float v1, uint32_t color,
                               const UITextureAtlas::Region &region);
    void EmitSliderNineSlice(BufferBuilder *builder, float x, float y, float width, float height,
                             float textureWidth, float textureHeight, float sourceBorderX,
                             float sourceBorderY, float destinationBorderX,
                             float destinationBorderY, uint32_t color,
                             const UITextureAtlas::Region &region);

    float GetNormalizedValue() const;
    float QuantizeValue(float value) const;
    bool UpdateValueFromActualX(int actualX, int actualWidth, int actualHeight);

protected:
    UIControl_Slider(float x, float y, float width, float height, const std::wstring &name,
                     float minValue, float maxValue, float defaultValue, float increment,
                     const char *backgroundTexturePath, const char *highlightTexturePath,
                     const char *graphicTexturePath);

    std::wstring m_name;
    std::wstring m_text;
    float m_value;
    float m_minValue;
    float m_maxValue;
    float m_defaultValue;
    float m_increment;
    float m_preferredWidth;
    float m_preferredHeight;
    bool m_dragging;
    UITextureAtlas::Region m_backgroundRegion;
    UITextureAtlas::Region m_highlightRegion;
    UITextureAtlas::Region m_graphicRegion;
};
