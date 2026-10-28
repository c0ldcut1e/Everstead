#pragma once

#include "Level.h"
#include "block/BlockSource.h"
#include "chunk/ChunkSource.h"

class LevelSource : public BlockSource, public ChunkSource
{
public:
    explicit LevelSource(const Level *level);

    uint32_t GetBlockId(const BlockPos &pos) const override;
    const Chunk *GetChunk(const ChunkPos &pos) const override;

private:
    const Level *m_level;
};
