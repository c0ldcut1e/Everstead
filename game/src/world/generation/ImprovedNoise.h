#pragma once

#include "../../utils/Random.h"

class ImprovedNoise
{
public:
    explicit ImprovedNoise(Random &random);

    double Noise(double x, double y, double z) const;

private:
    static double Fade(double t);
    static double Grad(int hash, double x, double y, double z);

    double m_offsetX;
    double m_offsetY;
    double m_offsetZ;
    int m_permutation[512];
};
