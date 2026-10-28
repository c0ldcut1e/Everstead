#pragma once

#include "WorldGenStage.h"

class Block;

class LavaHardenStage : public WorldGenStage
{
public:
    LavaHardenStage();

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    bool IsWater(const Chunk &chunk, int x, int y, int z) const;

    Block *m_obsidian;
    Block *m_cobblestone;
};
