#include "PerlinNoise.h"

PerlinNoise::PerlinNoise(Random &random, int octaves)
{
    m_levels.reserve(octaves);
    for (int i = 0; i < octaves; i++)
    {
        m_levels.emplace_back(random);
    }
}

double PerlinNoise::GetValue(double x, double y, double z) const
{
    double value = 0.0;
    double power = 1.0;

    for (const ImprovedNoise &level : m_levels)
    {
        value += level.Noise(x * power, y * power, z * power) / power;
        power /= 2.0;
    }

    return value;
}

double PerlinNoise::GetValue(double x, double z) const
{
    double value = 0.0;
    double power = 1.0;

    for (const ImprovedNoise &level : m_levels)
    {
        value += level.Noise(x * power, 0.0, z * power) / power;
        power /= 2.0;
    }

    return value;
}
