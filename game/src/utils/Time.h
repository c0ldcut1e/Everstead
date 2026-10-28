#pragma once

#include <cstdint>

class Time
{
public:
    static void Update();
    static double GetDelta();

    static uint32_t GetMilliseconds();

    static double GetTickDelta();
    static void SetTickDelta(double deltaTime);

private:
    static double s_delta;
    static double s_last;
    static double s_tickDelta;
};
