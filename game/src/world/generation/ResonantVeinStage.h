#pragma once

#include <vector>

#include "../block/BlockPos.h"
#include "WorldGenStage.h"

class Block;
class TerrainGenerator;

class ResonantVeinStage : public WorldGenStage
{
public:
    explicit ResonantVeinStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    bool IsSealed(Chunk &chunk, const BlockPos &pos) const;

    TerrainGenerator *m_generator;
    Block *m_crystal;
    std::vector<BlockPos> m_vein;
};
