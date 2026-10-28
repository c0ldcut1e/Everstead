#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_Checkbox.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"

const float UIControl_Checkbox::s_checkboxVirtualSize   = 28.0f;
const float UIControl_Checkbox::s_checkboxLabelScale    = 0.85f;
const float UIControl_Checkbox::s_checkboxLabelSpacing  = 4.0f;
const float UIControl_Checkbox::s_checkboxLabelYOffset  = 1.5f;
const float UIControl_Checkbox::s_checkboxDisabledAlpha = 0.6f;

void UIControl_Checkbox::EmitCheckboxQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                                          uint32_t color, const UITextureAtlas::Region &region)
{
    if (x1 <= x0 || y1 <= y0)
    {
        return;
    }

    builder->Color(color);
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y0, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y0, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v0));
    builder->VertexUV(x1, y1, 0.0f, UITextureAtlas::GetU(region, u1), UITextureAtlas::GetV(region, v1));
    builder->VertexUV(x0, y1, 0.0f, UITextureAtlas::GetU(region, u0), UITextureAtlas::GetV(region, v1));
}

UIControl_Checkbox::UIControl_Checkbox(float x, float y, float width, float height, const std::wstring &label, bool checked)
    : UIControl(x, y, width, height)
{
    m_label            = label;
    m_checked          = checked;
    m_preferredWidth   = std::max(width, 0.0f);
    m_preferredHeight  = std::max(height, 0.0f);
    m_backgroundRegion = UITextureAtlas::GetRegion("assets/texture/ui/control/tickbox_background.png");
    m_overRegion       = UITextureAtlas::GetRegion("assets/texture/ui/control/tickbox_over.png");
    m_tickRegion       = UITextureAtlas::GetRegion("assets/texture/ui/control/tick.png");
}

UIControl_Checkbox::~UIControl_Checkbox()
{}

void UIControl_Checkbox::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    int actualWidth  = everstead->GetWidth();
    int actualHeight = everstead->GetHeight();
    Font *font       = everstead->GetDefaultFont();

    Screen::Rect controlRect = GetActualRect(actualWidth, actualHeight);
    float uiScale            = Screen::ScaleUniform(actualWidth, actualHeight);
    if (uiScale <= 0.0f)
    {
        return;
    }

    float boxSize                                  = round(Screen::ToActualLength(s_checkboxVirtualSize, actualWidth, actualHeight));
    float boxX                                     = round(controlRect.x);
    float boxY                                     = round(controlRect.y + (controlRect.height - boxSize) * 0.5f);
    float contentAlpha                             = m_enabled ? 1.0f : s_checkboxDisabledAlpha;
    BufferBuilder *builder                         = Tesselator::GetInstance()->GetBuilder();
    const UITextureAtlas::Region &backgroundRegion = m_selected ? m_overRegion : m_backgroundRegion;

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    EmitCheckboxQuad(builder, boxX, boxY, boxX + boxSize, boxY + boxSize, 0.0f, 0.0f, 1.0f, 1.0f, ScaleColorAlpha(0xFFFFFFFF, contentAlpha),
                     backgroundRegion);
    builder->End();
    Textures::Unbind();

    if (m_checked)
    {
        float tickWidth = round(boxSize * ((float) m_tickRegion.pixelWidth / (float) m_tickRegion.pixelHeight));
        builder->Begin(ePrimitiveType_Triangles);
        Textures::Bind(UITextureAtlas::GetTexture()->GetId());
        GlStateManager::SetAlphaCutoff(0.5f);
        EmitCheckboxQuad(builder, boxX, boxY, boxX + tickWidth, boxY + boxSize, 0.0f, 0.0f, 1.0f, 1.0f, ScaleColorAlpha(0xFFFFFFFF, contentAlpha),
                         m_tickRegion);
        builder->End();
        Textures::Unbind();
    }

    GlStateManager::SetAlphaCutoff(-1.0f);

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();

    float fontScale        = font->SnapScale(uiScale * s_checkboxLabelScale);
    float labelSpacing     = round(Screen::ToActualLength(s_checkboxLabelSpacing, actualWidth, actualHeight));
    float labelYOffset     = round(Screen::ToActualLength(s_checkboxLabelYOffset, actualWidth, actualHeight));
    float textX            = round(boxX + boxSize + labelSpacing);
    float lineBottomOffset = font->GetLineHeight(fontScale) - font->GetAscent(fontScale);
    float textBaselineY    = round(boxY + boxSize - lineBottomOffset - labelYOffset);
    font->SetNearest(true);
    if (m_selected)
    {
        font->Draw(m_label, textX + 2.0f, textBaselineY + 2.0f, fontScale, ScaleColorAlpha(0xFF000000, contentAlpha));
    }

    font->Draw(m_label, textX, textBaselineY, fontScale, ScaleColorAlpha(m_selected ? 0xFFFFFF00 : 0xFF484848, contentAlpha));
}

float UIControl_Checkbox::GetPreferredWidth() const
{
    if (m_preferredWidth > 0.0f)
    {
        return m_preferredWidth;
    }

    return s_checkboxVirtualSize + s_checkboxLabelSpacing + MeasureLabelWidth();
}

float UIControl_Checkbox::GetPreferredHeight() const
{
    if (m_preferredHeight > 0.0f)
    {
        return m_preferredHeight;
    }

    return s_checkboxVirtualSize;
}

void UIControl_Checkbox::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_Checkbox::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

void UIControl_Checkbox::ResizeHorizontal(float)
{}

void UIControl_Checkbox::ResizeVertical(float)
{}

void UIControl_Checkbox::Activate()
{
    if (!m_enabled)
    {
        return;
    }

    m_checked = !m_checked;
}

bool UIControl_Checkbox::OnPointerPressed(int, int, int, int)
{
    Activate();
    return true;
}

void UIControl_Checkbox::SetChecked(bool checked)
{
    m_checked = checked;
}

bool UIControl_Checkbox::IsChecked() const
{
    return m_checked;
}

float UIControl_Checkbox::MeasureLabelWidth() const
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();
    return font->GetWidth(m_label, 1.0f);
}

#endif
