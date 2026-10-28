#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../rendering/BufferBuilder.h"
#include "../ChatText.h"
#include "UIComponent.h"
#include "UIComponent_Panel.h"

class UIComponent_Tooltip : public UIComponent
{
public:
    UIComponent_Tooltip();
    ~UIComponent_Tooltip() override;

    void Render() override;

    void SetText(const std::wstring &text);
    void SetLines(const std::vector<std::vector<ChatTextSegment>> &lines);
    void SetPosition(int actualX, int actualY);
    float GetWidth() const;
    float GetHeight() const;

private:
    static const int s_textHeight            = 8;
    static const int s_lineSpacing           = 10;
    static const int s_firstLineGap          = 2;
    static const int s_javaPadding           = 4;
    static const uint32_t s_javaBackground   = 0xF0100010;
    static const uint32_t s_javaBorderTop    = 0x505000FF;
    static const uint32_t s_javaBorderBottom = 0x5028007F;
    static const int s_legacyPadSide         = 10;
    static const int s_legacyPadTop          = 18;
    static const int s_legacyPadBottom       = 13;
    static const int s_legacyShadow          = 2;
    static const int s_legacySlice           = 5;
    static const int s_legacyTextureSize     = 16;
    static constexpr float kLegacyTexelScale = 3.0f / 7.0f;

    static float GetPixelSize();
    static float GetLegacyTexelSize();
    static float GetLegacyTextPixelSize();

    float GetTextWidth(float fontScale) const;
    int GetTextHeight() const;
    void DrawLines(float x, float y, float fontScale, float pixel, bool shadow) const;

    void RenderJava();
    void RenderLegacy();
    void EmitGradientRect(BufferBuilder *builder, float x0, float y0, float x1, float y1, uint32_t topColor, uint32_t bottomColor);

    UIComponent_Panel *m_legacyPanel;
    std::vector<std::vector<ChatTextSegment>> m_lines;
    int m_positionX;
    int m_positionY;
};
