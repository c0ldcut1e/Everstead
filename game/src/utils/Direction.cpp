#include "Direction.h"

// Pointers to static objects are constant-initialized, so tables in other files never copy a null direction
static Direction s_north("north", 0, 0, -1);
static Direction s_south("south", 0, 0, 1);
static Direction s_east("east", 1, 0, 0);
static Direction s_west("west", -1, 0, 0);
static Direction s_up("up", 0, 1, 0);
static Direction s_down("down", 0, -1, 0);

Direction *Direction::north = &s_north;
Direction *Direction::south = &s_south;
Direction *Direction::east  = &s_east;
Direction *Direction::west  = &s_west;
Direction *Direction::up    = &s_up;
Direction *Direction::down  = &s_down;

Direction::Direction(const char *_name, int _dx, int _dy, int _dz)
{
    name = _name;
    dx   = _dx;
    dy   = _dy;
    dz   = _dz;
}

uint8_t Direction::GetId() const
{
    if (this == Direction::north)
    {
        return 1;
    }

    if (this == Direction::south)
    {
        return 2;
    }

    if (this == Direction::west)
    {
        return 3;
    }

    if (this == Direction::east)
    {
        return 4;
    }

    if (this == Direction::down)
    {
        return 5;
    }

    if (this == Direction::up)
    {
        return 6;
    }

    return 0;
}

Direction *Direction::ById(uint8_t id)
{
    if (id == 1)
    {
        return Direction::north;
    }

    if (id == 2)
    {
        return Direction::south;
    }

    if (id == 3)
    {
        return Direction::west;
    }

    if (id == 4)
    {
        return Direction::east;
    }

    if (id == 5)
    {
        return Direction::down;
    }

    if (id == 6)
    {
        return Direction::up;
    }

    return nullptr;
}

Direction *Direction::Opposite(Direction *direction)
{
    if (direction == Direction::north)
    {
        return Direction::south;
    }

    if (direction == Direction::south)
    {
        return Direction::north;
    }

    if (direction == Direction::west)
    {
        return Direction::east;
    }

    if (direction == Direction::east)
    {
        return Direction::west;
    }

    if (direction == Direction::down)
    {
        return Direction::up;
    }

    if (direction == Direction::up)
    {
        return Direction::down;
    }

    return nullptr;
}
