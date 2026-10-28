#include "Mth.h"

#include <cmath>

int Mth::FloorDiv(int a, int b)
{
    int r = a / b;
    if ((a ^ b) < 0 && a % b)
    {
        r--;
    }

    return r;
}

int Mth::FloorMod(int a, int b)
{
    int m = a % b;
    if (m < 0)
    {
        m += b;
    }

    return m;
}

float Mth::WrapDegrees(float degrees)
{
    degrees = fmodf(degrees, 360.0f);
    if (degrees >= 180.0f)
    {
        degrees -= 360.0f;
    }

    if (degrees < -180.0f)
    {
        degrees += 360.0f;
    }

    return degrees;
}
