#include "LargeFeature.h"

LargeFeature::LargeFeature(uint32_t seed)
{
    m_seed = seed;
}

int LargeFeature::NextInt(Random &random, int bound) const
{
    if (bound <= 1)
    {
        return 0;
    }

    return random.NextInt(0, bound - 1);
}
