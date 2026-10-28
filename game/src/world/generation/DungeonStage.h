#pragma once

#include "../../utils/Random.h"
#include "WorldGenStage.h"

class Block;
class TerrainGenerator;

class DungeonStage : public WorldGenStage
{
public:
    explicit DungeonStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    void Place(Chunk &chunk, const ChunkPos &pos, Random &random, int x, int y, int z);
    void PlaceChest(Chunk &chunk, const ChunkPos &pos, Random &random, int x, int y, int z, int facing);

    TerrainGenerator *m_generator;
    Block *m_air;
    Block *m_cobblestone;
    Block *m_mossyCobblestone;
    Block *m_chest;
    Block *m_spawner;
};
