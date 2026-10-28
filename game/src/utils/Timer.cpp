#include "Timer.h"

#include <algorithm>

#include "Time.h"

Timer::Timer(float ticksPerSecond, float maxFrameSeconds)
{
    m_ticksPerSecond  = ticksPerSecond;
    m_tickDelta       = ticksPerSecond > 0.0f ? 1.0f / ticksPerSecond : 0.0f;
    m_maxFrameSeconds = maxFrameSeconds;
    m_accumulator     = 0.0;
    m_partialTicks    = 0.0f;
    m_elapsedTicks    = 0;
    m_frameDelta      = SmoothFloat((float) m_tickDelta);
    Time::SetTickDelta((float) m_tickDelta);
}

void Timer::AdvanceTime(double deltaSeconds)
{
    double frameSeconds = std::clamp(deltaSeconds, 0.0, m_maxFrameSeconds);
    m_frameDelta.Update((float) frameSeconds, 0.1f);

    m_elapsedTicks = 0;
    if (m_tickDelta <= 0.0f)
    {
        m_partialTicks = 0.0f;
        return;
    }

    m_accumulator += frameSeconds;
    while (m_accumulator >= m_tickDelta && m_elapsedTicks < 10)
    {
        m_accumulator -= m_tickDelta;
        m_elapsedTicks++;
    }

    m_partialTicks = (float) (m_accumulator / m_tickDelta);
    m_partialTicks = std::clamp(m_partialTicks, 0.0f, 1.0f);
}

void Timer::SetTicksPerSecond(float ticksPerSecond)
{
    m_ticksPerSecond = ticksPerSecond;
    m_tickDelta      = 1.0 / ticksPerSecond;
    Time::SetTickDelta((float) m_tickDelta);
}

int Timer::GetElapsedTicks() const
{
    return m_elapsedTicks;
}

float Timer::GetPartialTicks() const
{
    return m_partialTicks;
}

float Timer::GetFrameSeconds() const
{
    return m_frameDelta.GetValue();
}
