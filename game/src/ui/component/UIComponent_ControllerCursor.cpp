#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_ControllerCursor.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../UITextureAtlas.h"

UIComponent_ControllerCursor::UIComponent_ControllerCursor() : UIComponent()
{
    m_x    = 0.0f;
    m_y    = 0.0f;
    m_size = 0.0f;
}

UIComponent_ControllerCursor::~UIComponent_ControllerCursor()
{}

void UIComponent_ControllerCursor::SetPosition(float actualX, float actualY)
{
    m_x = actualX;
    m_y = actualY;
}

void UIComponent_ControllerCursor::SetSize(float actualSize)
{
    m_size = actualSize;
}

float UIComponent_ControllerCursor::GetSize() const
{
    return m_size;
}

void UIComponent_ControllerCursor::Render()
{
    if (!IsVisible())
    {
        return;
    }

    float size = roundf(m_size);
    float x    = roundf(m_x - size * 0.5f);
    float y    = roundf(m_y - size * 0.5f);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    UITextureAtlas::Region region = UITextureAtlas::GetRegion("assets/texture/ui/component/cursor.png");
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(0xFFFFFFFF);
    builder->VertexUV(x, y, 0.0f, region.u0, region.v0);
    builder->VertexUV(x + size, y, 0.0f, region.u1, region.v0);
    builder->VertexUV(x + size, y + size, 0.0f, region.u1, region.v1);
    builder->VertexUV(x, y, 0.0f, region.u0, region.v0);
    builder->VertexUV(x + size, y + size, 0.0f, region.u1, region.v1);
    builder->VertexUV(x, y + size, 0.0f, region.u0, region.v1);
    builder->End();
    Textures::Unbind();
}

#endif
