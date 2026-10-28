#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Image.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/BufferBuilder.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../Screen.h"
#include "../UITextureAtlas.h"

UIComponent_Image::UIComponent_Image(const std::string &texturePath) : UIComponent()
{
    m_region     = UITextureAtlas::GetRegion(texturePath);
    m_x          = 0.0f;
    m_y          = 0.0f;
    m_width      = 0.0f;
    m_height     = 0.0f;
    m_cropWidth  = 1.0f;
    m_cropHeight = 1.0f;
}

UIComponent_Image::~UIComponent_Image()
{}

void UIComponent_Image::Render()
{
    if (!IsVisible())
    {
        return;
    }

    Everstead *everstead = Everstead::GetInstance();
    Screen::Rect full    = Screen::ToActualRect(m_x, m_y, m_width, m_height, everstead->GetWidth(), everstead->GetHeight());
    float left           = roundf(full.x);
    float right          = roundf(full.x + full.width);
    float bottom         = roundf(full.y + full.height);
    float fullTop        = roundf(full.y);
    float top            = roundf(full.y + full.height * (1.0f - m_cropHeight));
    float cropRight      = roundf(full.x + full.width * m_cropWidth);
    Screen::Rect rect    = {left, top, cropRight - left, bottom - top};
    float u1             = m_region.u0 + (m_region.u1 - m_region.u0) * (cropRight - left) / (right - left);
    float v0             = m_region.v1 - (m_region.v1 - m_region.v0) * (bottom - top) / (bottom - fullTop);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(0xFFFFFFFF);
    builder->VertexUV(rect.x, rect.y, 0.0f, m_region.u0, v0);
    builder->VertexUV(rect.x + rect.width, rect.y, 0.0f, u1, v0);
    builder->VertexUV(rect.x + rect.width, rect.y + rect.height, 0.0f, u1, m_region.v1);
    builder->VertexUV(rect.x, rect.y, 0.0f, m_region.u0, v0);
    builder->VertexUV(rect.x + rect.width, rect.y + rect.height, 0.0f, u1, m_region.v1);
    builder->VertexUV(rect.x, rect.y + rect.height, 0.0f, m_region.u0, m_region.v1);
    builder->End();
    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

void UIComponent_Image::SetBounds(float x, float y, float width, float height)
{
    m_x      = x;
    m_y      = y;
    m_width  = width;
    m_height = height;
}

void UIComponent_Image::SetCrop(float width, float height)
{
    m_cropWidth  = width;
    m_cropHeight = height;
}

#endif
