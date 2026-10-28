#include "IceSpikeStage.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "../block/Block.h"
#include "TerrainGenerator.h"

static bool IsSpikeReplaceable(Block *block)
{
    static Block *s_air       = Block::ByName("air");
    static Block *s_dirt      = Block::ByName("dirt");
    static Block *s_grass     = Block::ByName("grass_block");
    static Block *s_snowBlock = Block::ByName("snow_block");
    static Block *s_ice       = Block::ByName("ice");
    return block == s_air || block == s_dirt || block == s_grass || block == s_snowBlock || block == s_ice;
}

static void PlaceSpikeBlock(Chunk &chunk, Block *packedIce, int chunkX, int chunkZ, int x, int y, int z, bool overIce)
{
    int localX = x - chunkX;
    int localZ = z - chunkZ;
    if (localX < 0 || localZ < 0 || localX >= Chunk::s_sizeX || localZ >= Chunk::s_sizeZ || y < 1 || y >= Chunk::s_sizeY)
    {
        return;
    }

    Block *block = Block::ById(chunk.GetBlockId(localX, y, localZ));
    if (IsSpikeReplaceable(block) || (overIce && block == packedIce))
    {
        chunk.SetBlock(localX, y, localZ, packedIce);
    }
}

IceSpikeStage::IceSpikeStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void IceSpikeStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int spikesPerChunk  = 3;
    const int patchesPerChunk = 2;

    // Giant spikes can lean on neighbours a few blocks away, so owners one chunk away catch every overlap
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dz = -1; dz <= 1; dz++)
        {
            ChunkPos owner(pos.x + dx, pos.z + dz);
            int ownerX = owner.x * Chunk::s_sizeX;
            int ownerZ = owner.z * Chunk::s_sizeZ;
            Random random(m_generator->GetChunkSeed(owner, 0x5851f42d4c957f2dULL));
            for (int i = 0; i < patchesPerChunk + spikesPerChunk; i++)
            {
                int levelX    = ownerX + random.NextInt(0, Chunk::s_sizeX - 1);
                int levelZ    = ownerZ + random.NextInt(0, Chunk::s_sizeZ - 1);
                uint64_t seed = random.NextUInt64();
                if (!m_generator->GetBiomeSource().GetBiome(levelX, levelZ)->HasIceSpikes())
                {
                    continue;
                }

                int surface = m_generator->GetTerrainHeight(levelX, levelZ);
                if (surface < TerrainGenerator::s_seaLevel)
                {
                    continue;
                }

                Random featureRandom(seed);
                if (i < patchesPerChunk)
                {
                    PlaceIcePatch(chunk, pos, featureRandom, levelX, surface, levelZ);
                }
                else
                {
                    PlaceIceSpike(chunk, pos, featureRandom, levelX, surface, levelZ);
                }
            }
        }
    }
}

void IceSpikeStage::PlaceIcePatch(Chunk &chunk, const ChunkPos &pos, Random &random, int levelX, int surface, int levelZ)
{
    Block *packedIce = Block::ByName("packed_ice");
    Block *air       = Block::ByName("air");
    int chunkX       = pos.x * Chunk::s_sizeX;
    int chunkZ       = pos.z * Chunk::s_sizeZ;
    int radius       = random.NextInt(2, 3);

    for (int dx = -radius; dx <= radius; dx++)
    {
        for (int dz = -radius; dz <= radius; dz++)
        {
            int localX = levelX + dx - chunkX;
            int localZ = levelZ + dz - chunkZ;
            if (dx * dx + dz * dz > radius * radius || localX < 0 || localZ < 0 || localX >= Chunk::s_sizeX || localZ >= Chunk::s_sizeZ)
            {
                continue;
            }

            for (int y = surface - 1; y <= surface + 1; y++)
            {
                Block *block = Block::ById(chunk.GetBlockId(localX, y, localZ));
                if (block != air && IsSpikeReplaceable(block))
                {
                    chunk.SetBlock(localX, y, localZ, packedIce);
                }
            }
        }
    }
}

void IceSpikeStage::PlaceIceSpike(Chunk &chunk, const ChunkPos &pos, Random &random, int levelX, int surface, int levelZ)
{
    const int giantChance  = 60;
    const int pillarLength = 50;
    const int pillarFloor  = TerrainGenerator::s_seaLevel - 13;
    const int fillerDepth  = 4;

    Block *packedIce = Block::ByName("packed_ice");
    int chunkX       = pos.x * Chunk::s_sizeX;
    int chunkZ       = pos.z * Chunk::s_sizeZ;

    int baseY  = surface + random.NextInt(0, 3);
    int height = random.NextInt(0, 3) + 7;
    int width  = height / 4 + random.NextInt(0, 1);
    if (width > 1 && random.NextInt(0, giantChance - 1) == 0)
    {
        baseY += random.NextInt(0, 29) + 10;
    }

    // The shape is walked in full every time so each chunk consumes the same random values and the pieces line up
    for (int y = 0; y < height; y++)
    {
        float scale = (1.0f - (float) y / (float) height) * (float) width;
        int reach   = (int) std::ceil(scale);
        for (int dx = -reach; dx <= reach; dx++)
        {
            float fx = (float) std::abs(dx) - 0.25f;
            for (int dz = -reach; dz <= reach; dz++)
            {
                float fz    = (float) std::abs(dz) - 0.25f;
                bool inside = (dx == 0 && dz == 0) || fx * fx + fz * fz <= scale * scale;
                bool edge   = dx == -reach || dx == reach || dz == -reach || dz == reach;
                if (inside && (!edge || random.NextFloat() <= 0.75f))
                {
                    PlaceSpikeBlock(chunk, packedIce, chunkX, chunkZ, levelX + dx, baseY + y, levelZ + dz, false);
                    if (y != 0 && reach > 1)
                    {
                        PlaceSpikeBlock(chunk, packedIce, chunkX, chunkZ, levelX + dx, baseY - y, levelZ + dz, false);
                    }
                }
            }
        }
    }

    // The pillar runs down through snow and dirt until it reaches stone, using terrain height so every chunk agrees on where it ends
    int pillar = std::clamp(width - 1, 0, 1);
    for (int dx = -pillar; dx <= pillar; dx++)
    {
        for (int dz = -pillar; dz <= pillar; dz++)
        {
            int stopY = std::max(pillarFloor, m_generator->GetTerrainHeight(levelX + dx, levelZ + dz) - fillerDepth);
            int run   = pillarLength;
            if (std::abs(dx) == 1 && std::abs(dz) == 1)
            {
                run = random.NextInt(0, 4);
            }

            int y = baseY - 1;
            while (y > stopY)
            {
                PlaceSpikeBlock(chunk, packedIce, chunkX, chunkZ, levelX + dx, y, levelZ + dz, true);
                y--;
                run--;
                if (run <= 0)
                {
                    y -= random.NextInt(0, 4) + 1;
                    run = random.NextInt(0, 4);
                }
            }
        }
    }
}
