#pragma once

#include "../chunk/Chunk.h"

class WorldGenStage
{
public:
    virtual ~WorldGenStage() = default;

    virtual void Apply(Chunk &chunk, const ChunkPos &pos) = 0;
};
