#include "ImprovedNoise.h"

#include <glm/common.hpp>

ImprovedNoise::ImprovedNoise(Random &random)
{
    m_offsetX = random.NextDouble() * 256.0;
    m_offsetY = random.NextDouble() * 256.0;
    m_offsetZ = random.NextDouble() * 256.0;

    for (int i = 0; i < 256; i++)
    {
        m_permutation[i] = i;
    }

    for (int i = 0; i < 256; i++)
    {
        int j                  = random.NextInt(0, 255 - i) + i;
        int tmp                = m_permutation[i];
        m_permutation[i]       = m_permutation[j];
        m_permutation[j]       = tmp;
        m_permutation[i + 256] = m_permutation[i];
    }
}

double ImprovedNoise::Fade(double t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double ImprovedNoise::Grad(int hash, double x, double y, double z)
{
    int h    = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double ImprovedNoise::Noise(double x, double y, double z) const
{
    x += m_offsetX;
    y += m_offsetY;
    z += m_offsetZ;

    int xf = (int) x;
    int yf = (int) y;
    int zf = (int) z;
    if (x < xf)
    {
        xf--;
    }
    if (y < yf)
    {
        yf--;
    }
    if (z < zf)
    {
        zf--;
    }

    int gridX = xf & 255;
    int gridY = yf & 255;
    int gridZ = zf & 255;

    x -= xf;
    y -= yf;
    z -= zf;

    double u = Fade(x);
    double v = Fade(y);
    double w = Fade(z);

    int a  = m_permutation[gridX] + gridY;
    int aa = m_permutation[a] + gridZ;
    int ab = m_permutation[a + 1] + gridZ;
    int b  = m_permutation[gridX + 1] + gridY;
    int ba = m_permutation[b] + gridZ;
    int bb = m_permutation[b + 1] + gridZ;

    return glm::mix(glm::mix(glm::mix(Grad(m_permutation[aa], x, y, z), Grad(m_permutation[ba], x - 1.0, y, z), u),
                             glm::mix(Grad(m_permutation[ab], x, y - 1.0, z), Grad(m_permutation[bb], x - 1.0, y - 1.0, z), u), v),
                    glm::mix(glm::mix(Grad(m_permutation[aa + 1], x, y, z - 1.0), Grad(m_permutation[ba + 1], x - 1.0, y, z - 1.0), u),
                             glm::mix(Grad(m_permutation[ab + 1], x, y - 1.0, z - 1.0), Grad(m_permutation[bb + 1], x - 1.0, y - 1.0, z - 1.0), u),
                             v),
                    w);
}
