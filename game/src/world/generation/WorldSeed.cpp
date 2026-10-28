#include "WorldSeed.h"

#include <cstdlib>
#include <random>

#include "../../utils/Time.h"

uint32_t WorldSeed::Generate()
{
    std::random_device randomDevice;
    uint32_t seedHigh = ((uint32_t) randomDevice()) << 16;
    uint32_t seedLow  = (uint32_t) randomDevice();
    uint32_t seed     = seedHigh ^ seedLow ^ (uint32_t) Time::GetMilliseconds();
    if (seed == 0)
    {
        seed = 1;
    }

    return seed;
}

uint32_t WorldSeed::Parse(const std::string &text)
{
    if (text.empty())
    {
        return Generate();
    }

    char *end;
    long long number = strtoll(text.c_str(), &end, 10);
    if (*end == '\0')
    {
        return (uint32_t) number;
    }

    uint32_t hash = 0;
    for (size_t i = 0; i < text.size(); i++)
    {
        hash = hash * 31 + (unsigned char) text[i];
    }

    return hash;
}
