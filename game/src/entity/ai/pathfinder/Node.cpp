#include "Node.h"

#include <cmath>
#include <cstdint>

Node::Node(int _x, int _y, int _z)
{
    x        = _x;
    y        = _y;
    z        = _z;
    g        = 0.0f;
    h        = 0.0f;
    f        = 0.0f;
    cameFrom = nullptr;
    open     = false;
    closed   = false;
}

float Node::DistanceTo(const Node *other) const
{
    float dx = (float) (other->x - x);
    float dy = (float) (other->y - y);
    float dz = (float) (other->z - z);
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

int Node::CreateHash(int x, int y, int z)
{
    uint32_t hash = ((uint32_t) y & 0xFF) | (((uint32_t) x & 0x7FFF) << 8) | (((uint32_t) z & 0x7FFF) << 24);
    if (x < 0)
    {
        hash |= 0x80000000;
    }

    if (z < 0)
    {
        hash |= 0x8000;
    }

    return (int) hash;
}
