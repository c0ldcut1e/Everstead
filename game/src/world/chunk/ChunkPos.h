#pragma once

#include <cstddef>
#include <functional>

struct ChunkPos
{
    ChunkPos()
    {
        x = 0;
        z = 0;
    }

    ChunkPos(int _x, int _z)
    {
        x = _x;
        z = _z;
    }

    bool operator==(const ChunkPos &other) const
    {
        return x == other.x && z == other.z;
    }

    bool operator!=(const ChunkPos &other) const
    {
        return !(*this == other);
    }

    ChunkPos operator+(const ChunkPos &other) const
    {
        return {x + other.x, z + other.z};
    }

    ChunkPos operator-(const ChunkPos &other) const
    {
        return {x - other.x, z - other.z};
    }

    ChunkPos &operator+=(const ChunkPos &other)
    {
        x += other.x;
        z += other.z;
        return *this;
    }

    ChunkPos &operator-=(const ChunkPos &other)
    {
        x -= other.x;
        z -= other.z;
        return *this;
    }

    bool operator<(const ChunkPos &other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }

        return z < other.z;
    }

    int x;
    int z;
};

struct ChunkPosHash
{
    size_t operator()(const ChunkPos &pos) const
    {
        size_t h1 = std::hash<int>()(pos.x);
        size_t h2 = std::hash<int>()(pos.z);
        return h1 ^ (h2 << 1);
    }
};
