#pragma once

#include <cstdint>

enum eDIMENSION_ID : uint8_t
{
    eDimensionId_Overworld = 0,
    eDimensionId_Caves     = 1,
    eDimensionId_Count,
};

inline constexpr const char *kDimensionNames[eDimensionId_Count] = {"overworld", "caves"};
