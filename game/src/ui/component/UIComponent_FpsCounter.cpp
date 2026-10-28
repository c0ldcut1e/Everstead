#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_FpsCounter.h"

#include <algorithm>
#include <cmath>
#include <cwchar>

#include "../../core/Everstead.h"
#include "../../core/GameInfo.h"
#include "../../rendering/Font.h"
#include "../../utils/Time.h"
#include "../Screen.h"
#include "../control/UIController.h"
#include "../scene/UIScene.h"

UIComponent_FpsCounter::UIComponent_FpsCounter() : UIComponent()
{
    m_frames       = 0;
    m_elapsed      = 0.0;
    m_minFrameTime = 0.0;
    m_maxFrameTime = 0.0;
    m_fps          = 0;
    m_minFps       = 0;
    m_maxFps       = 0;
    m_averageFps   = 0;
    m_historyCount = 0;
    m_historyIndex = 0;
    for (int i = 0; i < s_historySeconds; i++)
    {
        m_history[i] = 0;
    }
}

void UIComponent_FpsCounter::Tick()
{
    double delta = Time::GetDelta();
    if (m_frames == 0 || delta < m_minFrameTime)
    {
        m_minFrameTime = delta;
    }

    if (m_frames == 0 || delta > m_maxFrameTime)
    {
        m_maxFrameTime = delta;
    }

    m_frames++;
    m_elapsed += delta;
    if (m_elapsed < 1.0)
    {
        return;
    }

    m_fps                     = (int) lround((double) m_frames / m_elapsed);
    m_minFps                  = m_maxFrameTime > 0.0 ? (int) lround(1.0 / m_maxFrameTime) : m_fps;
    m_maxFps                  = m_minFrameTime > 0.0 ? (int) lround(1.0 / m_minFrameTime) : m_fps;
    m_history[m_historyIndex] = m_fps;
    m_historyIndex            = (m_historyIndex + 1) % s_historySeconds;
    m_historyCount            = std::min(m_historyCount + 1, s_historySeconds);

    int total = 0;
    for (int i = 0; i < m_historyCount; i++)
    {
        total += m_history[i];
    }

    m_averageFps = total / m_historyCount;
    m_frames     = 0;
    m_elapsed    = 0.0;
}

void UIComponent_FpsCounter::Render()
{
    Everstead *everstead = Everstead::GetInstance();
    int mode             = everstead->options.fpsDisplay;
    UIScene *debugScene  = everstead->GetUiController()->FindScene(eUIScene_DebugOverlay);
    if (!IsVisible() || (debugScene && debugScene->IsVisible()))
    {
        return;
    }

    float guiPixel  = Screen::GetGuiPixelSize(everstead->options.GetHudScale(), everstead->GetWidth(), everstead->GetHeight());
    Font *font      = everstead->GetDefaultFont();
    float fontScale = font->GetPixelPerfectScale(guiPixel);
    float margin    = (float) s_marginPixels * guiPixel;
    float baselineY = margin + font->GetAscent(fontScale);
    wchar_t text[128];
    swprintf(text, 128, L"Everstead %u.%u.%u", g_gameVersionMajor, g_gameVersionMinor, g_gameVersionPatch);
    font->SetNearest(true);
    font->DrawShadow(text, margin, baselineY, fontScale, 0xFFFFFFFF);
    if (mode == eFpsDisplay_Off)
    {
        return;
    }

    if (mode == eFpsDisplay_Extended)
    {
        swprintf(text, 128, L"%d FPS  min %d  avg %d  max %d", m_fps, m_minFps, m_averageFps, m_maxFps);
    }
    else
    {
        swprintf(text, 128, L"%d FPS", m_fps);
    }

    font->DrawShadow(text, (float) everstead->GetWidth() - margin - font->GetWidth(text, fontScale), baselineY, fontScale, 0xFFFFFFFF);
}

#endif
