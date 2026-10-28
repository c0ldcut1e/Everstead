#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Panel.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/math/Mth.h"
#include "../Screen.h"
#include "../UIPanelProperties.h"

uint32_t UIComponent_Panel::PackPanelColor(float transparency)
{
    uint32_t alpha = (uint32_t) (std::clamp(transparency, 0.0f, 1.0f) * 255.0f + 0.5f);
    return (alpha << 24) | 0x00FFFFFF;
}

void UIComponent_Panel::EmitPanelQuad(BufferBuilder *builder, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
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

UIComponent_Panel::UIComponent_Panel(std::unique_ptr<UIPanelProperties> properties) : UIComponent()
{
    m_properties = std::move(properties);
    m_region     = UITextureAtlas::GetRegion(m_properties ? m_properties->GetTexturePath() : "assets/texture/ui/control/panel9grid.png");
}

UIComponent_Panel::~UIComponent_Panel()
{}

void UIComponent_Panel::Render()
{
    if (!IsVisible() || !m_properties || m_properties->width <= 0.0f || m_properties->height <= 0.0f || m_properties->transparency <= 0.0f)
    {
        return;
    }

    Everstead *everstead = Everstead::GetInstance();

    int actualWidth   = everstead->GetWidth();
    int actualHeight  = everstead->GetHeight();
    int textureWidth  = m_region.pixelWidth;
    int textureHeight = m_region.pixelHeight;
    if (textureWidth <= 0 || textureHeight <= 0)
    {
        return;
    }

    float panelX      = Screen::ToActualX(m_properties->x, actualWidth, actualHeight);
    float panelY      = Screen::ToActualY(m_properties->y, actualWidth, actualHeight);
    float panelWidth  = Screen::ToActualLength(m_properties->width, actualWidth, actualHeight);
    float panelHeight = Screen::ToActualLength(m_properties->height, actualWidth, actualHeight);

    float sourceSliceX = std::max(1.0f, (float) round((float) textureWidth * m_properties->GetSliceRatio()));
    float sourceSliceY = std::max(1.0f, (float) round((float) textureHeight * m_properties->GetSliceRatio()));
    float sliceX       = Screen::ToActualLength(sourceSliceX * m_properties->cornerScale, actualWidth, actualHeight);
    float sliceY       = Screen::ToActualLength(sourceSliceY * m_properties->cornerScale, actualWidth, actualHeight);

    float left   = std::min(sliceX, panelWidth * 0.5f);
    float right  = std::min(sliceX, panelWidth - left);
    float top    = std::min(sliceY, panelHeight * 0.5f);
    float bottom = std::min(sliceY, panelHeight - top);

    float x0 = panelX;
    float x1 = panelX + left;
    float x2 = panelX + panelWidth - right;
    float x3 = panelX + panelWidth;

    float y0 = panelY;
    float y1 = panelY + top;
    float y2 = panelY + panelHeight - bottom;
    float y3 = panelY + panelHeight;

    float u0 = 0.0f;
    float u1 = sourceSliceX / (float) textureWidth;
    float u2 = ((float) textureWidth - sourceSliceX) / (float) textureWidth;
    float u3 = 1.0f;

    float v0 = 0.0f;
    float v1 = sourceSliceY / (float) textureHeight;
    float v2 = ((float) textureHeight - sourceSliceY) / (float) textureHeight;
    float v3 = 1.0f;

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    uint32_t color         = PackPanelColor(m_properties->transparency);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    EmitPanelQuad(builder, x0, y0, x1, y1, u0, v0, u1, v1, color, m_region);
    EmitPanelQuad(builder, x1, y0, x2, y1, u1, v0, u2, v1, color, m_region);
    EmitPanelQuad(builder, x2, y0, x3, y1, u2, v0, u3, v1, color, m_region);
    EmitPanelQuad(builder, x0, y1, x1, y2, u0, v1, u1, v2, color, m_region);
    EmitPanelQuad(builder, x1, y1, x2, y2, u1, v1, u2, v2, color, m_region);
    EmitPanelQuad(builder, x2, y1, x3, y2, u2, v1, u3, v2, color, m_region);
    EmitPanelQuad(builder, x0, y2, x1, y3, u0, v2, u1, v3, color, m_region);
    EmitPanelQuad(builder, x1, y2, x2, y3, u1, v2, u2, v3, color, m_region);
    EmitPanelQuad(builder, x2, y2, x3, y3, u2, v2, u3, v3, color, m_region);
    builder->End();
    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

UIPanelProperties *UIComponent_Panel::GetProperties()
{
    return m_properties.get();
}

const UIPanelProperties *UIComponent_Panel::GetProperties() const
{
    return m_properties.get();
}

#endif
