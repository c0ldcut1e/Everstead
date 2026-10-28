#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_IconLabel.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "../../core/Everstead.h"
#include "../../io/File.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"

const float UIControl_IconLabel::s_iconLabelDefaultWidth = 360.0f;
const float UIControl_IconLabel::s_iconLabelTextScale    = 0.85f;
const float UIControl_IconLabel::s_iconLabelIconPaddingX = 6.0f;
const float UIControl_IconLabel::s_iconLabelIconSize     = 64.0f;
const float UIControl_IconLabel::s_iconLabelTextPaddingX = 12.0f;

void UIControl_IconLabel::EmitIconLabelQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
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

UIControl_IconLabel::UIControl_IconLabel(float x, float y, float width, float height, const std::wstring &label, const char *iconTexturePath,
                                         const char *fallbackIconTexturePath)
    : UIControl(x, y, width, height)
{
    m_label           = label;
    m_iconRegion      = UITextureAtlas::GetRegion(fallbackIconTexturePath);
    m_iconTexture     = nullptr;
    m_preferredWidth  = width > 0.0f ? width : s_iconLabelDefaultWidth;
    m_preferredHeight = height > 0.0f ? height : s_iconLabelDefaultHeight;
    m_hasIconTexture  = false;

    if (iconTexturePath && fallbackIconTexturePath && strcmp(iconTexturePath, fallbackIconTexturePath) != 0 && File(iconTexturePath).IsRegularFile())
    {
        try
        {
            m_iconTexture = TextureRepository::GetInstance()->Get(iconTexturePath);
            m_hasIconTexture =
                    m_iconTexture && m_iconTexture->GetId().IsValid() && m_iconTexture->GetPixelWidth() > 0 && m_iconTexture->GetPixelHeight() > 0;
        }
        catch (...)
        {
            m_iconTexture    = nullptr;
            m_hasIconTexture = false;
        }
    }
}

UIControl_IconLabel::~UIControl_IconLabel()
{}

void UIControl_IconLabel::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();

    int actualWidth   = everstead->GetWidth();
    int actualHeight  = everstead->GetHeight();
    Screen::Rect rect = GetActualRect(actualWidth, actualHeight);
    float uiScale     = Screen::ScaleUniform(actualWidth, actualHeight);

    float iconSize     = round(Screen::ToActualLength(s_iconLabelIconSize, actualWidth, actualHeight));
    float iconPaddingX = round(Screen::ToActualLength(s_iconLabelIconPaddingX, actualWidth, actualHeight));
    float iconX        = round(rect.x + iconPaddingX);
    float iconY        = round(rect.y + (rect.height - iconSize) * 0.5f);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::SetAlphaCutoff(0.01f);

    if (!m_hasIconTexture)
    {
        builder->Begin(ePrimitiveType_Triangles);
        Textures::Bind(UITextureAtlas::GetTexture()->GetId());
        EmitIconLabelQuad(builder, iconX, iconY, iconX + iconSize, iconY + iconSize, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, m_iconRegion);
        builder->End();
        Textures::Unbind();
    }
    else
    {
        builder->Begin(ePrimitiveType_Triangles);
        Textures::Bind(m_iconTexture->GetId());
        builder->Color(0xFFFFFFFF);
        builder->VertexUV(iconX, iconY, 0.0f, 0.0f, 0.0f);
        builder->VertexUV(iconX + iconSize, iconY, 0.0f, 1.0f, 0.0f);
        builder->VertexUV(iconX + iconSize, iconY + iconSize, 0.0f, 1.0f, 1.0f);
        builder->VertexUV(iconX, iconY, 0.0f, 0.0f, 0.0f);
        builder->VertexUV(iconX + iconSize, iconY + iconSize, 0.0f, 1.0f, 1.0f);
        builder->VertexUV(iconX, iconY + iconSize, 0.0f, 0.0f, 1.0f);
        builder->End();
        Textures::Unbind();
    }

    GlStateManager::SetAlphaCutoff(-1.0f);

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();

    if (!m_label.empty())
    {
        float fontScale     = font->SnapScale(uiScale * s_iconLabelTextScale);
        float textPaddingX  = round(Screen::ToActualLength(s_iconLabelTextPaddingX, actualWidth, actualHeight));
        float textX         = round(iconX + iconSize + textPaddingX);
        float textBaselineY = round(rect.y + (rect.height - font->GetLineHeight(fontScale)) * 0.5f + font->GetAscent(fontScale));

        font->SetNearest(true);
        font->Draw(m_label, textX, textBaselineY, fontScale, s_iconLabelTextColor);
    }
}

float UIControl_IconLabel::GetPreferredWidth() const
{
    return m_preferredWidth;
}

float UIControl_IconLabel::GetPreferredHeight() const
{
    return m_preferredHeight;
}

void UIControl_IconLabel::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_IconLabel::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

const std::wstring &UIControl_IconLabel::GetLabel() const
{
    return m_label;
}

void UIControl_IconLabel::SetLabel(const std::wstring &label)
{
    m_label = label;
}

#endif
