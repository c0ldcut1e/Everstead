#include "Random.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <numbers>

uint64_t Random::Splitmix64(uint64_t &state)
{
    uint64_t z = (state += 0x9E3779B97f4A7C15);
    z          = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z          = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

uint64_t Random::SeedFromSystem()
{
    static std::atomic<uint64_t> s_seedUniquifier(0x1ED8B55FAC9DEC6E);

    uint64_t uniquifier = s_seedUniquifier.fetch_add(0x9E3779B97F4A7C15);
    return Splitmix64(uniquifier) ^ (uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

Random::Random()
{
    m_state = SeedFromSystem();
    Splitmix64(m_state);
}

Random::Random(uint64_t seed)
{
    m_state = seed ? seed : 0xDEADDEADDEADDEAD;
    Splitmix64(m_state);
}

uint64_t Random::GetState() const
{
    return m_state;
}

void Random::SetState(uint64_t state)
{
    m_state = state;
}

uint64_t Random::NextUInt64()
{
    return Splitmix64(m_state);
}

uint32_t Random::NextUInt()
{
    return (uint32_t) (NextUInt64() >> 32);
}

int Random::NextInt(int min, int max)
{
    return min + (int) (NextUInt() % (uint32_t) (max - min + 1));
}

float Random::NextFloat()
{
    return (float) (NextUInt() / (double) UINT32_MAX);
}

double Random::NextDouble()
{
    return (double) (NextUInt() / (double) UINT32_MAX);
}

double Random::NextGaussian()
{
    double u = std::max(NextDouble(), 0.000001);
    double v = NextDouble();
    return std::sqrt(-2.0 * std::log(u)) * std::cos(2.0 * std::numbers::pi * v);
}
