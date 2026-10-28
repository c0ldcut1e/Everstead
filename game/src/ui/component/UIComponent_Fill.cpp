#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Fill.h"

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"

UIComponent_Fill::UIComponent_Fill(uint32_t color) : UIComponent()
{
    m_color = color;
}

UIComponent_Fill::~UIComponent_Fill()
{}

void UIComponent_Fill::Render()
{
    Everstead *everstead = Everstead::GetInstance();

    float width  = (float) everstead->GetWidth();
    float height = (float) everstead->GetHeight();

    Tesselator *tesselator = Tesselator::GetInstance();
    BufferBuilder *builder = tesselator->GetBuilder();

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(m_color);
    builder->Vertex(0.0f, height, 0.0f);
    builder->Vertex(width, height, 0.0f);
    builder->Vertex(width, 0.0f, 0.0f);
    builder->Vertex(0.0f, height, 0.0f);
    builder->Vertex(width, 0.0f, 0.0f);
    builder->Vertex(0.0f, 0.0f, 0.0f);
    builder->End();

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void UIComponent_Fill::SetColor(uint32_t color)
{
    m_color = color;
}

uint32_t UIComponent_Fill::GetColor() const
{
    return m_color;
}

#endif
