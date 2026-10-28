#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIControllerGlyphRenderer.h"

#include "../core/Everstead.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/Tesselator.h"
#include "../rendering/Textures.h"
#include "UITextureAtlas.h"

float UIControllerGlyphRenderer::GetWidth(eCONTROLLER_BUTTON button, float size)
{
    UITextureAtlas::Region region =
            UITextureAtlas::GetRegion(ControllerGlyphs::GetIconPath(button, Everstead::GetInstance()->GetInputManager()->GetControllerType()));
    return size * (float) region.pixelWidth / (float) region.pixelHeight;
}

float UIControllerGlyphRenderer::Render(eCONTROLLER_BUTTON button, float x, float y, float size)
{
    UITextureAtlas::Region region =
            UITextureAtlas::GetRegion(ControllerGlyphs::GetIconPath(button, Everstead::GetInstance()->GetInputManager()->GetControllerType()));
    float width = size * (float) region.pixelWidth / (float) region.pixelHeight;

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(0xFFFFFFFF);
    builder->VertexUV(x, y, 0.0f, region.u0, region.v0);
    builder->VertexUV(x + width, y, 0.0f, region.u1, region.v0);
    builder->VertexUV(x + width, y + size, 0.0f, region.u1, region.v1);
    builder->VertexUV(x, y, 0.0f, region.u0, region.v0);
    builder->VertexUV(x + width, y + size, 0.0f, region.u1, region.v1);
    builder->VertexUV(x, y + size, 0.0f, region.u0, region.v1);
    builder->End();
    Textures::Unbind();
    return width;
}

#endif
