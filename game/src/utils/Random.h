#pragma once

#include <cstdint>

class Random
{
public:
    Random();
    explicit Random(uint64_t seed);

    uint32_t NextUInt();
    uint64_t NextUInt64();
    int NextInt(int min, int max);
    float NextFloat();
    double NextDouble();
    double NextGaussian();
    uint64_t GetState() const;
    void SetState(uint64_t state);

private:
    uint64_t m_state;

    static uint64_t SeedFromSystem();
    static uint64_t Splitmix64(uint64_t &state);
};
