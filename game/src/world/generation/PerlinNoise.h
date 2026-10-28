#pragma once

#include <vector>

#include "../../utils/Random.h"
#include "ImprovedNoise.h"

class PerlinNoise
{
public:
    PerlinNoise(Random &random, int octaves);

    double GetValue(double x, double y, double z) const;
    double GetValue(double x, double z) const;

private:
    std::vector<ImprovedNoise> m_levels;
};
