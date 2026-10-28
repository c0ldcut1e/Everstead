#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_StatusBars.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../rendering/Font.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../../utils/Random.h"
#include "../../world/Level.h"
#include "../Screen.h"
#include "UIComponent_Hotbar.h"

UIComponent_StatusBars::UIComponent_StatusBars() : UIComponent()
{
    m_originX = 0.0f;
    m_originY = 0.0f;
    m_scale   = 1.0f;
}

float UIComponent_StatusBars::GetTopY(int width, int height, float hudScale)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (!localPlayer || !GameType::CanHurtPlayer(localPlayer->GetGameType()))
    {
        return UIComponent_Hotbar::GetTopY(width, height, hudScale);
    }

    return UIComponent_Hotbar::GetY(width, height, hudScale) + round((float) s_secondRowYOffset * Screen::ScaleUniform(width, height) * hudScale);
}

void UIComponent_StatusBars::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    if (!everstead->options.displayHud || everstead->IsWorldMenuOpen())
    {
        return;
    }

    LocalPlayer *localPlayer = everstead->GetLocalPlayer();
    if (!localPlayer || !GameType::CanHurtPlayer(localPlayer->GetGameType()))
    {
        return;
    }

    int actualWidth  = everstead->GetWidth();
    int actualHeight = everstead->GetHeight();
    float hudScale   = Screen::SnapHudScale(everstead->options.GetHudScale(), actualWidth, actualHeight);
    m_scale          = Screen::ScaleUniform(actualWidth, actualHeight) * hudScale;
    m_originX        = round((float) actualWidth * 0.5f - round((float) s_hotbarWidth * m_scale) * 0.5f);
    m_originY        = UIComponent_Hotbar::GetY(actualWidth, actualHeight, hudScale);

    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    UITextureAtlas::Region heartContainer       = UITextureAtlas::GetRegion("assets/texture/ui/hud/hearts/normal_background.png");
    UITextureAtlas::Region heartFull            = UITextureAtlas::GetRegion("assets/texture/ui/hud/hearts/normal_full.png");
    UITextureAtlas::Region heartHalf            = UITextureAtlas::GetRegion("assets/texture/ui/hud/hearts/normal_half.png");
    UITextureAtlas::Region heartFlashing        = UITextureAtlas::GetRegion("assets/texture/ui/hud/hearts/normal_background_flashing.png");
    UITextureAtlas::Region heartFullPrevious    = UITextureAtlas::GetRegion("assets/texture/ui/hud/hearts/normal_full_previous.png");
    UITextureAtlas::Region heartHalfPrevious    = UITextureAtlas::GetRegion("assets/texture/ui/hud/hearts/normal_half_previous.png");
    UITextureAtlas::Region foodEmpty            = UITextureAtlas::GetRegion("assets/texture/ui/hud/food/background.png");
    UITextureAtlas::Region foodFull             = UITextureAtlas::GetRegion("assets/texture/ui/hud/food/full.png");
    UITextureAtlas::Region foodHalf             = UITextureAtlas::GetRegion("assets/texture/ui/hud/food/half.png");
    UITextureAtlas::Region armorEmpty           = UITextureAtlas::GetRegion("assets/texture/ui/hud/armor/empty.png");
    UITextureAtlas::Region armorFull            = UITextureAtlas::GetRegion("assets/texture/ui/hud/armor/full.png");
    UITextureAtlas::Region armorHalf            = UITextureAtlas::GetRegion("assets/texture/ui/hud/armor/half.png");
    UITextureAtlas::Region airBubble            = UITextureAtlas::GetRegion("assets/texture/ui/hud/air/full_bubble.png");
    UITextureAtlas::Region airBubblePopping     = UITextureAtlas::GetRegion("assets/texture/ui/hud/air/popping_bubble.png");
    UITextureAtlas::Region experienceBackground = UITextureAtlas::GetRegion("assets/texture/ui/hud/experience/background.png");
    UITextureAtlas::Region experienceFill       = UITextureAtlas::GetRegion("assets/texture/ui/hud/experience/fill.png");

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(UITextureAtlas::GetTexture()->GetId());
    builder->Color(0xFFFFFFFF);

    DrawSprite(builder, experienceBackground, 0.0f, (float) s_experienceYOffset, (float) s_hotbarWidth);
    int experienceWidth = (int) (localPlayer->GetExperienceProgress() * (float) (s_hotbarWidth + 1));
    if (experienceWidth > 0)
    {
        DrawSprite(builder, experienceFill, 0.0f, (float) s_experienceYOffset, (float) std::min(experienceWidth, s_hotbarWidth));
    }

    int health           = localPlayer->GetHealth();
    int lastHealth       = localPlayer->GetLastHealth();
    int food             = localPlayer->GetFoodData()->GetFoodLevel();
    int armor            = localPlayer->GetArmorValue();
    int invulnerableTime = localPlayer->GetInvulnerableTime();
    bool blink           = invulnerableTime >= Player::s_invulnerableDuration / 2 && invulnerableTime / 3 % 2 == 1;
    Level *level         = localPlayer->GetLevel();
    Random random((uint64_t) ((level ? level->GetGameTime() : 0) * s_shakeSeedMultiplier));

    int hearts    = (localPlayer->GetMaxHealth() + 1) / 2;
    int rows      = (hearts + 9) / 10;
    int rowHeight = std::max(10 - (rows - 2), 3);
    for (int i = 0; i < 10 && armor > 0; i++)
    {
        const UITextureAtlas::Region *armorIcon = &armorEmpty;
        if (i * 2 + 1 < armor)
        {
            armorIcon = &armorFull;
        }
        else if (i * 2 + 1 == armor)
        {
            armorIcon = &armorHalf;
        }

        DrawSprite(builder, *armorIcon, (float) (i * s_iconSpacing), (float) (s_secondRowYOffset - (rows - 1) * rowHeight), (float) s_iconSize);
    }

    for (int i = hearts - 1; i >= 0; i--)
    {
        float x = (float) (i % 10 * s_iconSpacing);
        float y = (float) (s_firstRowYOffset - i / 10 * rowHeight);
        if (health <= s_lowHealth)
        {
            y += (float) random.NextInt(0, 1);
        }

        DrawSprite(builder, blink ? heartFlashing : heartContainer, x, y, (float) s_iconSize);
        if (blink)
        {
            if (i * 2 + 1 < lastHealth)
            {
                DrawSprite(builder, heartFullPrevious, x, y, (float) s_iconSize);
            }
            else if (i * 2 + 1 == lastHealth)
            {
                DrawSprite(builder, heartHalfPrevious, x, y, (float) s_iconSize);
            }
        }

        if (i * 2 + 1 < health)
        {
            DrawSprite(builder, heartFull, x, y, (float) s_iconSize);
        }
        else if (i * 2 + 1 == health)
        {
            DrawSprite(builder, heartHalf, x, y, (float) s_iconSize);
        }
    }

    for (int i = 0; i < FoodData::kMaxFoodLevel / 2; i++)
    {
        float x = (float) (s_hotbarWidth - i * s_iconSpacing - s_iconSize);
        float y = (float) s_firstRowYOffset;
        DrawSprite(builder, foodEmpty, x, y, (float) s_iconSize);
        if (i * 2 + 1 < food)
        {
            DrawSprite(builder, foodFull, x, y, (float) s_iconSize);
        }
        else if (i * 2 + 1 == food)
        {
            DrawSprite(builder, foodHalf, x, y, (float) s_iconSize);
        }
    }

    if (localPlayer->IsEyeInWater())
    {
        int airSupply = localPlayer->GetAirSupply();
        int count     = (int) ceil((float) (airSupply - 2) * 10.0f / (float) Player::s_totalAirSupply);
        int extra     = (int) ceil((float) airSupply * 10.0f / (float) Player::s_totalAirSupply) - count;
        for (int i = 0; i < count + extra; i++)
        {
            float x = (float) (s_hotbarWidth - i * s_iconSpacing - s_iconSize);
            DrawSprite(builder, i < count ? airBubble : airBubblePopping, x, (float) s_secondRowYOffset, (float) s_iconSize);
        }
    }

    builder->End();
    Textures::Unbind();

    int experienceLevel = localPlayer->GetExperienceLevel();
    if (experienceLevel > 0)
    {
        Font *font        = everstead->GetDefaultFont();
        float pixel       = round(m_scale);
        float fontScale   = font->GetPixelPerfectScale(pixel);
        std::wstring text = std::to_wstring(experienceLevel);
        float textX       = round((float) actualWidth * 0.5f - font->GetWidth(text, fontScale) * 0.5f);
        float textY       = round(m_originY + (float) (s_experienceYOffset + 1) * m_scale);
        font->SetNearest(true);
        font->Draw(text, textX + pixel, textY, fontScale, 0xFF000000);
        font->Draw(text, textX - pixel, textY, fontScale, 0xFF000000);
        font->Draw(text, textX, textY + pixel, fontScale, 0xFF000000);
        font->Draw(text, textX, textY - pixel, fontScale, 0xFF000000);
        font->Draw(text, textX, textY, fontScale, 0xFF80FF20);
    }

    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableDepthTest();
}

void UIComponent_StatusBars::DrawSprite(BufferBuilder *builder, const UITextureAtlas::Region &region, float guiX, float guiY, float guiWidth)
{
    float x0 = round(m_originX + guiX * m_scale);
    float y0 = round(m_originY + guiY * m_scale);
    float x1 = round(m_originX + (guiX + guiWidth) * m_scale);
    float y1 = round(m_originY + (guiY + (float) region.pixelHeight) * m_scale);
    float u1 = UITextureAtlas::GetU(region, guiWidth / (float) region.pixelWidth);

    builder->VertexUV(x0, y0, 0.0f, region.u0, region.v0);
    builder->VertexUV(x1, y0, 0.0f, u1, region.v0);
    builder->VertexUV(x1, y1, 0.0f, u1, region.v1);
    builder->VertexUV(x0, y0, 0.0f, region.u0, region.v0);
    builder->VertexUV(x1, y1, 0.0f, u1, region.v1);
    builder->VertexUV(x0, y1, 0.0f, region.u0, region.v1);
}

#endif
