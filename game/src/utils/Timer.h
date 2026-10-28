#pragma once

#include "SmoothFloat.h"

class Timer
{
public:
    Timer(float ticksPerSecond, float maxFrameSeconds);

    void AdvanceTime(double deltaSeconds);
    void SetTicksPerSecond(float ticksPerSecond);

    int GetElapsedTicks() const;
    float GetPartialTicks() const;
    float GetFrameSeconds() const;

private:
    double m_ticksPerSecond;
    double m_tickDelta;
    double m_maxFrameSeconds;
    double m_accumulator;
    float m_partialTicks;
    int m_elapsedTicks;
    SmoothFloat m_frameDelta;
};
