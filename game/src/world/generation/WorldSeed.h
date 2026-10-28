#pragma once

#include <cstdint>
#include <string>

class WorldSeed
{
public:
    static uint32_t Generate();
    static uint32_t Parse(const std::string &text);
};
