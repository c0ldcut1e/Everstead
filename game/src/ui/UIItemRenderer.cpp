#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIItemRenderer.h"

#include <cmath>
#include <string>

#include "../core/Everstead.h"
#include "../rendering/BufferBuilder.h"
#include "../rendering/Font.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/Icon.h"
#include "../rendering/ItemAtlas.h"
#include "../rendering/Tesselator.h"
#include "../rendering/Texture.h"
#include "../rendering/Textures.h"
#include "../world/block/Block.h"
#include "../world/block/BlockRenderer.h"
#include "../world/fracture/FractureTraits.h"
#include "../world/item/Item.h"
#include "../world/item/ItemInstance.h"

const float UIItemRenderer::s_blockIconScale = 0.55f;

void UIItemRenderer::DrawItem(const ItemInstance *item, float x, float y, float width, float height)
{
    if (!item || item->IsEmpty() || !item->GetItem())
    {
        return;
    }

    Block *block = item->GetItem()->GetBlock();
    if (block && block->RendersAsItemBlock())
    {
        GlStateManager::PushMatrix();
        GlStateManager::Translatef(x, y, 0.0f);
        GlStateManager::Scalef(width, height, 1.0f);
        GlStateManager::Translatef(0.5f, 0.5f, 0.0f);
        GlStateManager::Scalef(s_blockIconScale, s_blockIconScale, -1.0f);
        GlStateManager::Rotatef(210.0f, 1.0f, 0.0f, 0.0f);
        GlStateManager::Rotatef(-45.0f, 0.0f, 1.0f, 0.0f);
        GlStateManager::SetGlint(FractureTraits::GetGlint(*item));
        BlockRenderer::RenderBlockAsItem(block, 0xFFFFFFFF);
        GlStateManager::SetGlint(0.0f);
        GlStateManager::PopMatrix();
        return;
    }

    const Icon &icon = item->GetIcon();
    if (icon.IsEmpty() || !icon.GetTexture())
    {
        return;
    }

    ItemAtlas::GetInstance()->GetAtlas()->MarkVisible({icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()});

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableCull();
    GlStateManager::DisableDepthTest();
    uint32_t tint = block && block->HasBiomeTint(Direction::north) ? block->GetItemTint() : 0xFFFFFF;
    GlStateManager::SetGlint(FractureTraits::GetGlint(*item));
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(icon.GetTexture()->GetId());
    builder->Color(0xFF000000 | tint);
    builder->VertexUV(x, y, 0.0f, icon.GetU0(), icon.GetV0());
    builder->VertexUV(x + width, y, 0.0f, icon.GetU1(), icon.GetV0());
    builder->VertexUV(x + width, y + height, 0.0f, icon.GetU1(), icon.GetV1());
    builder->VertexUV(x, y, 0.0f, icon.GetU0(), icon.GetV0());
    builder->VertexUV(x + width, y + height, 0.0f, icon.GetU1(), icon.GetV1());
    builder->VertexUV(x, y + height, 0.0f, icon.GetU0(), icon.GetV1());
    builder->End();
    GlStateManager::SetGlint(0.0f);
}

void UIItemRenderer::DrawItemCount(const ItemInstance *item, float x, float y, float size)
{
    float scale = size / 16.0f;
    DrawDurability(item, x, y, scale);

    if (item->GetCount() <= 1)
    {
        return;
    }

    Font *font        = Everstead::GetInstance()->GetDefaultFont();
    std::wstring text = std::to_wstring(item->GetCount());
    float fontScale   = font->GetPixelPerfectScale(scale);

    GlStateManager::DisableDepthTest();
    font->SetNearest(true);
    font->DrawShadow(text, roundf(x + size + scale - font->GetWidth(text, fontScale)), roundf(y + 9.0f * scale) + font->GetAscent(fontScale),
                     fontScale, 0xFFFFFFFF);
}

void UIItemRenderer::DrawDurability(const ItemInstance *item, float x, float y, float scale)
{
    Item *baseItem = item->GetItem();
    if (!baseItem || !baseItem->IsDamageable() || item->GetAuxValue() <= 0)
    {
        return;
    }

    float fraction = 1.0f - (float) item->GetAuxValue() / (float) baseItem->GetMaxDamage();
    int red        = (int) std::round(255.0f * (1.0f - fraction));
    int green      = (int) std::round(255.0f * fraction);
    uint32_t color = 0xFF000000 | ((uint32_t) red << 16) | ((uint32_t) green << 8);
    float barX     = x + 2.0f * scale;
    float barY     = y + 13.0f * scale;

    GlStateManager::DisableDepthTest();
    DrawRect(barX, barY, 13.0f * scale, 2.0f * scale, 0xFF000000);
    DrawRect(barX, barY, std::round(13.0f * fraction) * scale, scale, color);
}

void UIItemRenderer::DrawRect(float x, float y, float width, float height, uint32_t color)
{
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(color);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y + height, 0.0f);
    builder->Vertex(x + width, y, 0.0f);
    builder->Vertex(x, y, 0.0f);
    builder->End();
}

#endif
