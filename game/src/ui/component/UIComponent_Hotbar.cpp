#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Hotbar.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../../core/Everstead.h"
#include "../../entity/Inventory.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Icon.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/Time.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRenderer.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../Screen.h"
#include "../UIItemRenderer.h"
#include "../UITextureAtlas.h"

const float UIComponent_Hotbar::s_blockIconScale = 0.55f;

const float UIComponent_Hotbar::s_bottomOffset = 30.0f;

const float UIComponent_Hotbar::s_popDuration = 0.25f;

UIComponent_Hotbar::UIComponent_Hotbar() : UIComponent()
{
    m_hotbarRegion   = UITextureAtlas::GetRegion("assets/texture/ui/hud/hotbar/hotbar.png");
    m_selectedRegion = UITextureAtlas::GetRegion("assets/texture/ui/hud/hotbar/selected.png");
    m_nameSlot       = -1;
    m_nameItem       = nullptr;
    m_nameTime       = 0.0f;
}

// This is the top edge of the selected slot frame, which sits one texel above the bar
float UIComponent_Hotbar::GetTopY(int width, int height, float hudScale)
{
    float uiScale      = Screen::ScaleUniform(width, height);
    float scale        = uiScale * hudScale;
    float hotbarHeight = round((float) s_hotbarPixelHeight * scale);
    float hotbarY0     = (float) height - hotbarHeight - round(s_bottomOffset * uiScale);
    return round(hotbarY0 - hotbarHeight / (float) s_hotbarPixelHeight);
}

float UIComponent_Hotbar::GetItemNameBaselineY(int width, int height, float hudScale)
{
    float uiScale            = Screen::ScaleUniform(width, height);
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (localPlayer && GameType::CanHurtPlayer(localPlayer->GetGameType()))
    {
        return round(GetTopY(width, height, hudScale) - (18.0f * hudScale + 32.0f) * uiScale);
    }

    return round(GetTopY(width, height, hudScale) - (8.0f * hudScale + 16.0f) * uiScale);
}

float UIComponent_Hotbar::GetItemNameTopY(int width, int height, float hudScale)
{
    Font *font = Everstead::GetInstance()->GetDefaultFont();
    return GetItemNameBaselineY(width, height, hudScale) - font->GetAscent(GetItemNameFontScale(width, height, hudScale));
}

float UIComponent_Hotbar::GetItemNameFontScale(int width, int height, float hudScale)
{
    return Everstead::GetInstance()->GetDefaultFont()->GetPixelPerfectScale(floorf(Screen::ScaleUniform(width, height) * hudScale));
}

float UIComponent_Hotbar::GetY(int width, int height, float hudScale)
{
    float uiScale = Screen::ScaleUniform(width, height);
    return (float) height - round((float) s_hotbarPixelHeight * uiScale * hudScale) - round(s_bottomOffset * uiScale);
}

void UIComponent_Hotbar::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    if (!everstead->options.displayHud || everstead->IsWorldMenuOpen())
    {
        return;
    }

    LocalPlayer *localPlayer = everstead->GetLocalPlayer();
    if (!localPlayer || GameType::IsSpectator(localPlayer->GetGameType()))
    {
        return;
    }

    int actualWidth  = everstead->GetWidth();
    int actualHeight = everstead->GetHeight();
    float hudScale   = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    float scale      = Screen::ScaleUniform(actualWidth, actualHeight) * hudScale;

    float hotbarWidth  = round((float) m_hotbarRegion.pixelWidth * scale);
    float hotbarHeight = round((float) m_hotbarRegion.pixelHeight * scale);
    float hotbarX0     = round((float) actualWidth * 0.5f - hotbarWidth * 0.5f);
    float hotbarY0     = (float) actualHeight - hotbarHeight - round(s_bottomOffset * Screen::ScaleUniform(actualWidth, actualHeight));

    float texelScaleX = hotbarWidth / (float) m_hotbarRegion.pixelWidth;
    float texelScaleY = hotbarHeight / (float) m_hotbarRegion.pixelHeight;

    int selectedSlot     = localPlayer->GetInventory()->GetSelectedSlot();
    float selectedWidth  = round((float) m_selectedRegion.pixelWidth * texelScaleX);
    float selectedHeight = round((float) m_selectedRegion.pixelHeight * texelScaleY);
    float selectedX0     = round(hotbarX0 + ((float) (selectedSlot * s_slotWidth) - 1.0f) * texelScaleX);
    float selectedY0     = round(hotbarY0 - 1.0f * texelScaleY);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(0xFFFFFFFF);
    builder->VertexUV(hotbarX0, hotbarY0, 0.0f, m_hotbarRegion.u0, m_hotbarRegion.v0);
    builder->VertexUV(hotbarX0 + hotbarWidth, hotbarY0, 0.0f, m_hotbarRegion.u1, m_hotbarRegion.v0);
    builder->VertexUV(hotbarX0 + hotbarWidth, hotbarY0 + hotbarHeight, 0.0f, m_hotbarRegion.u1, m_hotbarRegion.v1);
    builder->VertexUV(hotbarX0, hotbarY0, 0.0f, m_hotbarRegion.u0, m_hotbarRegion.v0);
    builder->VertexUV(hotbarX0 + hotbarWidth, hotbarY0 + hotbarHeight, 0.0f, m_hotbarRegion.u1, m_hotbarRegion.v1);
    builder->VertexUV(hotbarX0, hotbarY0 + hotbarHeight, 0.0f, m_hotbarRegion.u0, m_hotbarRegion.v1);
    builder->VertexUV(selectedX0, selectedY0, 0.0f, m_selectedRegion.u0, m_selectedRegion.v0);
    builder->VertexUV(selectedX0 + selectedWidth, selectedY0, 0.0f, m_selectedRegion.u1, m_selectedRegion.v0);
    builder->VertexUV(selectedX0 + selectedWidth, selectedY0 + selectedHeight, 0.0f, m_selectedRegion.u1, m_selectedRegion.v1);
    builder->VertexUV(selectedX0, selectedY0, 0.0f, m_selectedRegion.u0, m_selectedRegion.v0);
    builder->VertexUV(selectedX0 + selectedWidth, selectedY0 + selectedHeight, 0.0f, m_selectedRegion.u1, m_selectedRegion.v1);
    builder->VertexUV(selectedX0, selectedY0 + selectedHeight, 0.0f, m_selectedRegion.u0, m_selectedRegion.v1);
    builder->End();

    Inventory *inventory = localPlayer->GetInventory();
    float delta          = (float) Time::GetDelta();
    float iconWidth      = round((float) s_iconSize * texelScaleX);
    float iconHeight     = round((float) s_iconSize * texelScaleY);
    for (int slot = 0; slot < Inventory::s_hotbarSize; slot++)
    {
        ItemInstance *item = inventory->GetItem(slot);
        if (!item || item->IsEmpty())
        {
            continue;
        }

        float iconX0 = round(hotbarX0 + ((float) (slot * s_slotWidth + s_iconInset)) * texelScaleX);
        float iconY0 = round(hotbarY0 + (float) s_iconInset * texelScaleY);
        float pop    = inventory->GetPopTime(slot) / s_popDuration;
        if (pop > 0.0f)
        {
            float width  = iconWidth / (1.0f + pop);
            float height = iconHeight * (1.0f + pop * 0.5f);
            UIItemRenderer::DrawItem(item, iconX0 + (iconWidth - width) * 0.5f, iconY0 + (iconHeight - height) * 0.75f, width, height);
            inventory->SetPopTime(slot, std::max(0.0f, inventory->GetPopTime(slot) - delta));
        }
        else
        {
            UIItemRenderer::DrawItem(item, iconX0, iconY0, iconWidth, iconHeight);
        }
    }

    Textures::Unbind();

    for (int slot = 0; slot < Inventory::s_hotbarSize; slot++)
    {
        ItemInstance *item = inventory->GetItem(slot);
        if (!item || item->IsEmpty())
        {
            continue;
        }

        float iconX0 = round(hotbarX0 + ((float) (slot * s_slotWidth + s_iconInset)) * texelScaleX);
        float iconY0 = round(hotbarY0 + (float) s_iconInset * texelScaleY);
        UIItemRenderer::DrawItemCount(item, iconX0, iconY0, iconWidth);
    }

    ItemInstance *selectedItem = inventory->GetSelectedItem();
    Item *heldItem             = selectedItem->IsEmpty() ? nullptr : selectedItem->GetItem();
    if (selectedSlot != m_nameSlot || heldItem != m_nameItem)
    {
        m_nameSlot = selectedSlot;
        m_nameItem = heldItem;
        m_nameTime = heldItem ? 2.0f : 0.0f;
    }

    if (m_nameTime > 0.0f)
    {
        Font *font        = everstead->GetDefaultFont();
        float fontScale   = GetItemNameFontScale(actualWidth, actualHeight, hudScale);
        std::wstring name = everstead->GetString(heldItem->GetDescriptionId());
        float textX       = round((float) actualWidth * 0.5f - font->GetWidth(name, fontScale) * 0.5f);
        float textY       = GetItemNameBaselineY(actualWidth, actualHeight, hudScale);
        uint32_t alpha    = (uint32_t) (std::min(m_nameTime / 0.5f, 1.0f) * 255.0f);
        font->SetNearest(true);
        font->DrawShadow(name, textX, textY, fontScale, (alpha << 24) | 0xFFFFFF);
        m_nameTime -= delta;
    }

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

#endif
