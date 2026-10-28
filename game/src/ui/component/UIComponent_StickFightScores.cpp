#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_StickFightScores.h"

#include <array>
#include <string>

#include "../../core/Everstead.h"
#include "../../rendering/Font.h"
#include "../Screen.h"

UIComponent_StickFightScores::UIComponent_StickFightScores() : UIComponent()
{}

void UIComponent_StickFightScores::Render()
{
    constexpr uint32_t kTeamColors[] = {0xFF5555FF, 0xFFFF5555, 0xFF55FF55, 0xFFFFFF55};
    Everstead *everstead             = Everstead::GetInstance();
    const std::array<int, 4> &scores = everstead->GetStickFightScores();
    float guiPixel                   = Screen::GetGuiPixelSize(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    Font *font                       = everstead->GetDefaultFont();
    float fontScale                  = font->GetPixelPerfectScale(guiPixel) * 2.0f;
    float spacing                    = font->GetWidth(L"   ", fontScale);
    std::array<std::wstring, 4> texts;
    float totalWidth = 0.0f;
    for (size_t i = 0; i < scores.size(); i++)
    {
        if (scores[i] >= 0)
        {
            texts[i] = std::to_wstring(scores[i]);
            totalWidth += font->GetWidth(texts[i], fontScale) + spacing;
        }
    }

    float x = ((float) everstead->GetWidth() - totalWidth + spacing) * 0.5f;
    float y = 4.0f * guiPixel + font->GetAscent(fontScale);
    font->SetNearest(true);
    for (size_t i = 0; i < scores.size(); i++)
    {
        if (scores[i] >= 0)
        {
            font->DrawShadow(texts[i], x, y, fontScale, kTeamColors[i]);
            x += font->GetWidth(texts[i], fontScale) + spacing;
        }
    }
}

#endif
