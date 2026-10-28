#pragma once

#include "../../utils/Random.h"
#include "../chunk/Chunk.h"

class Block;

class VeinFeature
{
public:
    static void Place(Chunk &chunk, const ChunkPos &pos, Random &random, int x, int y, int z, Block *block, Block *target, int size);
};
