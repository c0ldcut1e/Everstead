#pragma once

#include <cstdint>

#include "../../utils/Random.h"
#include "WorldGenStage.h"

class LargeFeature : public WorldGenStage
{
public:
    explicit LargeFeature(uint32_t seed);

protected:
    int NextInt(Random &random, int bound) const;

    uint32_t m_seed;
};
