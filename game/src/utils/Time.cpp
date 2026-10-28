#include "Time.h"

#include <chrono>

double Time::s_delta     = 0.0;
double Time::s_last      = 0.0;
double Time::s_tickDelta = 0.05;

void Time::Update()
{
    double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

    if (s_last == 0.0)
    {
        s_last  = now;
        s_delta = 0.0;
        return;
    }

    s_delta = now - s_last;
    s_last  = now;
}

double Time::GetDelta()
{
    return s_delta;
}

uint32_t Time::GetMilliseconds()
{
    return (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

double Time::GetTickDelta()
{
    return s_tickDelta;
}

void Time::SetTickDelta(double deltaTime)
{
    s_tickDelta = deltaTime;
}
