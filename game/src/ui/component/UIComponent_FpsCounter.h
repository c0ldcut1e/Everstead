#pragma once

#include "UIComponent.h"

class UIComponent_FpsCounter : public UIComponent
{
public:
    UIComponent_FpsCounter();

    void Tick() override;
    void Render() override;

private:
    static inline const int s_historySeconds = 5;
    static const int s_marginPixels          = 2;

    int m_frames;
    double m_elapsed;
    double m_minFrameTime;
    double m_maxFrameTime;
    int m_fps;
    int m_minFps;
    int m_maxFps;
    int m_averageFps;
    int m_history[s_historySeconds];
    int m_historyCount;
    int m_historyIndex;
};
