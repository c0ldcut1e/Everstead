#pragma once

#include <cstdint>
#include <string>

class Direction
{
public:
    Direction(const char *_name, int _dx, int _dy, int _dz);

    static Direction *north;
    static Direction *south;
    static Direction *east;
    static Direction *west;
    static Direction *up;
    static Direction *down;

    uint8_t GetId() const;

    static Direction *ById(uint8_t id);
    static Direction *Opposite(Direction *direction);

    std::string name;
    int dx;
    int dy;
    int dz;
};
