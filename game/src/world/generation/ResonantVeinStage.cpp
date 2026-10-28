#include "ResonantVeinStage.h"

#include "../../utils/Random.h"
#include "../block/Block.h"
#include "TerrainGenerator.h"

ResonantVeinStage::ResonantVeinStage(TerrainGenerator *generator)
{
    m_generator = generator;
    m_crystal   = Block::ByName("resonant_crystal");
}

// We only place a vein where every crystal is fully walled in, so caves never expose one
void ResonantVeinStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int chunkChance = 24;
    const int attempts    = 6;
    const int minY        = 5;
    const int maxY        = 16;

    Random random(m_generator->GetChunkSeed(pos, 0x7f3a9c2be14d6a05ULL));
    if (random.NextInt(0, chunkChance - 1) != 0)
    {
        return;
    }

    for (int attempt = 0; attempt < attempts; attempt++)
    {
        BlockPos start(random.NextInt(1, Chunk::s_sizeX - 2), random.NextInt(minY, maxY), random.NextInt(1, Chunk::s_sizeZ - 2));
        int dx     = random.NextInt(-1, 1);
        int dz     = random.NextInt(-1, 1);
        int dy     = dx == 0 && dz == 0 ? 1 : random.NextInt(-1, 1);
        int length = random.NextInt(3, 6);

        m_vein.clear();
        for (int i = 0; i < length; i++)
        {
            m_vein.push_back(BlockPos(start.x + dx * i, start.y + dy * i, start.z + dz * i));
        }

        if (length >= 4)
        {
            const BlockPos &fork = m_vein[length / 2];
            m_vein.push_back(BlockPos(fork.x + (dz != 0 ? 1 : 0), fork.y + 1, fork.z + (dz != 0 ? 0 : 1)));
        }

        bool sealed = true;
        for (size_t i = 0; i < m_vein.size() && sealed; i++)
        {
            sealed = IsSealed(chunk, m_vein[i]);
        }

        if (!sealed)
        {
            continue;
        }

        for (size_t i = 0; i < m_vein.size(); i++)
        {
            chunk.SetBlock(m_vein[i].x, m_vein[i].y, m_vein[i].z, m_crystal);
            chunk.SetBlockData(m_vein[i].x, m_vein[i].y, m_vein[i].z, 0);
        }

        return;
    }
}

bool ResonantVeinStage::IsSealed(Chunk &chunk, const BlockPos &pos) const
{
    const int offsets[7][3] = {{0, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int i = 0; i < 7; i++)
    {
        int x = pos.x + offsets[i][0];
        int y = pos.y + offsets[i][1];
        int z = pos.z + offsets[i][2];
        if (x < 0 || x >= Chunk::s_sizeX || y < 1 || y >= Chunk::s_sizeY || z < 0 || z >= Chunk::s_sizeZ)
        {
            return false;
        }

        Block *block = Block::ById(chunk.GetBlockId(x, y, z));
        if (!block->IsSolid() || block->IsTransparent() || block->IsLiquid() || block == m_crystal)
        {
            return false;
        }
    }

    return true;
}
