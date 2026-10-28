#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControl_IconButton.h"
#include "../../utils/Time.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SDL2/SDL.h>

#include "../../core/Everstead.h"
#include "../../io/File.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"

const float UIControl_IconButton::s_iconButtonDefaultWidth  = 520.0f;
const float UIControl_IconButton::s_iconButtonDefaultHeight = 60.0f;
const float UIControl_IconButton::s_iconButtonTextScale     = 0.8f;
const float UIControl_IconButton::s_iconButtonIconPaddingX  = 14.0f;
const float UIControl_IconButton::s_iconButtonIconSize      = 42.0f;
const float UIControl_IconButton::s_iconButtonTextPaddingX  = 12.0f;

void UIControl_IconButton::EmitIconButtonQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
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

UIControl_IconButton::UIControl_IconButton(float x, float y, float width, float height, const std::wstring &label, const char *normalTexturePath,
                                           const char *highlightTexturePath, const char *iconTexturePath, const char *fallbackIconTexturePath)
    : UIControl(x, y, width, height)
{
    m_label           = label;
    m_normalRegion    = UITextureAtlas::GetRegion(normalTexturePath);
    m_highlightRegion = UITextureAtlas::GetRegion(highlightTexturePath);
    m_iconRegion      = UITextureAtlas::GetRegion(fallbackIconTexturePath);
    m_iconTexture     = nullptr;
    m_preferredWidth  = width > 0.0f ? width : s_iconButtonDefaultWidth;
    m_preferredHeight = height > 0.0f ? height : s_iconButtonDefaultHeight;
    m_wasPressed      = false;
    m_hasIconTexture  = false;
    m_pressedAtMs     = 0;

    if (iconTexturePath && fallbackIconTexturePath && std::string(iconTexturePath) != std::string(fallbackIconTexturePath) &&
        File(iconTexturePath).IsRegularFile())
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

UIControl_IconButton::~UIControl_IconButton()
{}

void UIControl_IconButton::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    Font *font           = everstead->GetDefaultFont();

    int actualWidth   = everstead->GetWidth();
    int actualHeight  = everstead->GetHeight();
    Screen::Rect rect = GetActualRect(actualWidth, actualHeight);
    float uiScale     = Screen::ScaleUniform(actualWidth, actualHeight);

    float displayWidth  = round(Screen::ToActualLength(m_preferredWidth, actualWidth, actualHeight));
    float btnX          = round(rect.x + (rect.width - displayWidth) * 0.5f);
    float btnY          = round(rect.y);
    float displayHeight = round(rect.y + rect.height) - btnY;

    BufferBuilder *builder               = Tesselator::GetInstance()->GetBuilder();
    const UITextureAtlas::Region &region = m_selected ? m_highlightRegion : m_normalRegion;

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    GlStateManager::SetAlphaCutoff(0.01f);
    EmitIconButtonQuad(builder, btnX, btnY, btnX + displayWidth, btnY + displayHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, region);

    float iconSize     = round(Screen::ToActualLength(s_iconButtonIconSize, actualWidth, actualHeight));
    float iconPaddingX = round(Screen::ToActualLength(s_iconButtonIconPaddingX, actualWidth, actualHeight));
    float iconX        = round(btnX + iconPaddingX);
    float iconY        = round(btnY + (displayHeight - iconSize) * 0.5f);
    if (!m_hasIconTexture)
    {
        EmitIconButtonQuad(builder, iconX, iconY, iconX + iconSize, iconY + iconSize, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, m_iconRegion);
    }

    builder->End();
    Textures::Unbind();

    if (m_hasIconTexture)
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
        float fontScale     = font->SnapScale(uiScale * s_iconButtonTextScale);
        float textPaddingX  = round(Screen::ToActualLength(s_iconButtonTextPaddingX, actualWidth, actualHeight));
        float textX         = round(iconX + iconSize + textPaddingX);
        float textBaselineY = round(btnY + (displayHeight - font->GetLineHeight(fontScale)) * 0.5f + font->GetAscent(fontScale));
        bool flashing       = m_wasPressed && (Time::GetMilliseconds() - m_pressedAtMs) < 500u;
        uint32_t textColor  = (flashing || !m_selected) ? s_iconButtonDefaultTextColor : s_iconButtonSelectedTextColor;

        font->SetNearest(true);
        font->Draw(m_label, textX + 2.0f, textBaselineY + 2.0f, fontScale, 0xFF000000);
        font->Draw(m_label, textX, textBaselineY, fontScale, textColor);
    }
}

float UIControl_IconButton::GetPreferredWidth() const
{
    return m_preferredWidth;
}

float UIControl_IconButton::GetPreferredHeight() const
{
    return m_preferredHeight;
}

void UIControl_IconButton::SetPreferredWidth(float preferredWidth)
{
    m_preferredWidth = std::max(preferredWidth, 0.0f);
}

void UIControl_IconButton::SetPreferredHeight(float preferredHeight)
{
    m_preferredHeight = std::max(preferredHeight, 0.0f);
}

void UIControl_IconButton::Activate()
{
    m_wasPressed  = true;
    m_pressedAtMs = Time::GetMilliseconds();
}

bool UIControl_IconButton::OnPointerPressed(int, int, int, int)
{
    m_wasPressed  = true;
    m_pressedAtMs = Time::GetMilliseconds();
    return true;
}

const std::wstring &UIControl_IconButton::GetLabel() const
{
    return m_label;
}

void UIControl_IconButton::SetLabel(const std::wstring &label)
{
    m_label = label;
}

#endif
