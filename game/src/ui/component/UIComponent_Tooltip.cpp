#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Tooltip.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../Gui.h"
#include "../Screen.h"
#include "../UIPanelProperties.h"

class UIPanelProperties_Tooltip : public UIPanelProperties
{
public:
    UIPanelProperties_Tooltip(float sliceRatio) : UIPanelProperties(0.0f, 0.0f, 0.0f, 0.0f)
    {
        m_sliceRatio = sliceRatio;
    }

    const char *GetTexturePath() const override
    {
        return "assets/texture/ui/control/tooltip9grid.png";
    }

    float GetSliceRatio() const override
    {
        return m_sliceRatio;
    }

private:
    float m_sliceRatio;
};

UIComponent_Tooltip::UIComponent_Tooltip() : UIComponent()
{
    m_positionX   = 0;
    m_positionY   = 0;
    m_legacyPanel = new UIComponent_Panel(std::make_unique<UIPanelProperties_Tooltip>((float) s_legacySlice / (float) s_legacyTextureSize));
}

UIComponent_Tooltip::~UIComponent_Tooltip()
{
    delete m_legacyPanel;
}

void UIComponent_Tooltip::Render()
{
    if (!IsVisible() || m_lines.empty())
    {
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    if (everstead->options.javaTooltips)
    {
        RenderJava();
        return;
    }

    RenderLegacy();
}

void UIComponent_Tooltip::SetText(const std::wstring &text)
{
    m_lines.clear();
    if (!text.empty())
    {
        m_lines.push_back({{text, ChatTextStyle()}});
    }
}

void UIComponent_Tooltip::SetLines(const std::vector<std::vector<ChatTextSegment>> &lines)
{
    m_lines = lines;
}

void UIComponent_Tooltip::SetPosition(int actualX, int actualY)
{
    m_positionX = actualX;
    m_positionY = actualY;
}

float UIComponent_Tooltip::GetPixelSize()
{
    Everstead *everstead = Everstead::GetInstance();
    return Screen::GetGuiPixelSize(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
}

float UIComponent_Tooltip::GetLegacyTexelSize()
{
    return std::max(1.0f, roundf(GetPixelSize() * kLegacyTexelScale));
}

float UIComponent_Tooltip::GetLegacyTextPixelSize()
{
    return GetLegacyTexelSize() * 2.0f;
}

float UIComponent_Tooltip::GetTextWidth(float fontScale) const
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    float width          = 0.0f;
    for (size_t i = 0; i < m_lines.size(); i++)
    {
        float lineWidth = 0.0f;
        for (size_t j = 0; j < m_lines[i].size(); j++)
        {
            lineWidth += everstead->GetGui()->GetTextWidth(font, m_lines[i][j], fontScale);
        }

        width = std::max(width, lineWidth);
    }

    return width;
}

int UIComponent_Tooltip::GetTextHeight() const
{
    if (m_lines.size() <= 1)
    {
        return s_textHeight;
    }

    return s_textHeight + s_firstLineGap + (int) (m_lines.size() - 1) * s_lineSpacing;
}

void UIComponent_Tooltip::DrawLines(float x, float y, float fontScale, float pixel, bool shadow) const
{
    Everstead *everstead = Everstead::GetInstance();
    Gui *gui             = everstead->GetGui();
    Font *font           = everstead->GetDefaultFont();
    font->SetNearest(true);
    float lineY = y + font->GetAscent(fontScale);
    for (size_t i = 0; i < m_lines.size(); i++)
    {
        float segmentX = x;
        for (size_t j = 0; j < m_lines[i].size(); j++)
        {
            gui->DrawTextSegment(font, m_lines[i][j], segmentX, lineY, fontScale, 1.0f, shadow);
            segmentX += gui->GetTextWidth(font, m_lines[i][j], fontScale);
        }

        lineY += (float) (i == 0 ? s_lineSpacing + s_firstLineGap : s_lineSpacing) * pixel;
    }
}

float UIComponent_Tooltip::GetWidth() const
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    if (everstead->options.javaTooltips)
    {
        float p = GetPixelSize();
        return GetTextWidth(font->GetPixelPerfectScale(p)) + (float) (s_javaPadding * 2) * p;
    }

    return GetTextWidth(font->GetPixelPerfectScale(GetLegacyTextPixelSize())) + (float) (s_legacyPadSide * 2) * GetLegacyTexelSize();
}

float UIComponent_Tooltip::GetHeight() const
{
    if (Everstead::GetInstance()->options.javaTooltips)
    {
        return (float) (GetTextHeight() + s_javaPadding * 2) * GetPixelSize();
    }

    return (float) (s_legacyPadTop + s_legacyPadBottom) * GetLegacyTexelSize() + (float) GetTextHeight() * GetLegacyTextPixelSize();
}

void UIComponent_Tooltip::RenderJava()
{
    Everstead *everstead = Everstead::GetInstance();
    float p              = GetPixelSize();
    Font *font           = everstead->GetDefaultFont();
    float fontScale      = font->GetPixelPerfectScale(p);
    float width          = GetTextWidth(fontScale);
    float height         = (float) GetTextHeight() * p;
    float x              = (float) m_positionX + (float) s_javaPadding * p;
    float y              = (float) m_positionY + (float) s_javaPadding * p;

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    Textures::Unbind();

    builder->Begin(ePrimitiveType_Triangles);
    EmitGradientRect(builder, x - 3 * p, y - 4 * p, x + width + 3 * p, y - 3 * p, s_javaBackground, s_javaBackground);
    EmitGradientRect(builder, x - 3 * p, y + height + 3 * p, x + width + 3 * p, y + height + 4 * p, s_javaBackground, s_javaBackground);
    EmitGradientRect(builder, x - 3 * p, y - 3 * p, x + width + 3 * p, y + height + 3 * p, s_javaBackground, s_javaBackground);
    EmitGradientRect(builder, x - 4 * p, y - 3 * p, x - 3 * p, y + height + 3 * p, s_javaBackground, s_javaBackground);
    EmitGradientRect(builder, x + width + 3 * p, y - 3 * p, x + width + 4 * p, y + height + 3 * p, s_javaBackground, s_javaBackground);
    EmitGradientRect(builder, x - 3 * p, y - 2 * p, x - 2 * p, y + height + 2 * p, s_javaBorderTop, s_javaBorderBottom);
    EmitGradientRect(builder, x + width + 2 * p, y - 2 * p, x + width + 3 * p, y + height + 2 * p, s_javaBorderTop, s_javaBorderBottom);
    EmitGradientRect(builder, x - 3 * p, y - 3 * p, x + width + 3 * p, y - 2 * p, s_javaBorderTop, s_javaBorderTop);
    EmitGradientRect(builder, x - 3 * p, y + height + 2 * p, x + width + 3 * p, y + height + 3 * p, s_javaBorderBottom, s_javaBorderBottom);
    builder->End();

    GlStateManager::EnableCull();
    DrawLines(x, y, fontScale, p, true);
}

void UIComponent_Tooltip::RenderLegacy()
{
    Everstead *everstead          = Everstead::GetInstance();
    int actualWidth               = everstead->GetWidth();
    int actualHeight              = everstead->GetHeight();
    float scale                   = Screen::ScaleUniform(actualWidth, actualHeight);
    float texel                   = GetLegacyTexelSize();
    Font *font                    = everstead->GetDefaultFont();
    float p                       = GetLegacyTextPixelSize();
    float fontScale               = font->GetPixelPerfectScale(p);
    float boxX                    = (float) m_positionX;
    float boxY                    = (float) m_positionY;
    float boxHeight               = GetHeight();
    UIPanelProperties *properties = m_legacyPanel->GetProperties();
    properties->x                 = Screen::ToVirtualX(boxX, actualWidth, actualHeight);
    properties->y                 = Screen::ToVirtualY(boxY, actualWidth, actualHeight);
    properties->width             = (GetWidth() + (float) s_legacyShadow * texel) / scale;
    properties->height            = (boxHeight + (float) s_legacyShadow * texel) / scale;
    properties->cornerScale       = texel / scale;
    m_legacyPanel->Render();

    DrawLines(boxX + (float) s_legacyPadSide * texel, boxY + (float) s_legacyPadTop * texel, fontScale, p, false);
}

void UIComponent_Tooltip::EmitGradientRect(BufferBuilder *builder, float x0, float y0, float x1, float y1, uint32_t topColor, uint32_t bottomColor)
{
    builder->Color(topColor);
    builder->Vertex(x0, y0, 0.0f);
    builder->Vertex(x1, y0, 0.0f);
    builder->Color(bottomColor);
    builder->Vertex(x1, y1, 0.0f);
    builder->Color(topColor);
    builder->Vertex(x0, y0, 0.0f);
    builder->Color(bottomColor);
    builder->Vertex(x1, y1, 0.0f);
    builder->Vertex(x0, y1, 0.0f);
}

#endif
