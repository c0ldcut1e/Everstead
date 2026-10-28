#pragma once

#include <cstddef>
#include <functional>

#include "ChunkPos.h"

struct SectionPos
{
    SectionPos()
    {
        x = 0;
        y = 0;
        z = 0;
    }

    SectionPos(int sectionX, int sectionY, int sectionZ)
    {
        x = sectionX;
        y = sectionY;
        z = sectionZ;
    }

    ChunkPos ToChunkPos() const
    {
        return ChunkPos(x, z);
    }

    bool operator==(const SectionPos &other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const SectionPos &other) const
    {
        return !(*this == other);
    }

    int x;
    int y;
    int z;
};

struct SectionPosHash
{
    size_t operator()(const SectionPos &pos) const
    {
        size_t h1 = std::hash<int>()(pos.x);
        size_t h2 = std::hash<int>()(pos.y);
        size_t h3 = std::hash<int>()(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
