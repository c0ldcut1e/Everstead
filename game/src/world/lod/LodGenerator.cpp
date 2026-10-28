#include "LodGenerator.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

#include "../../core/EversteadServer.h"
#include "../../threading/ThreadPool.h"
#include "../../utils/math/Mth.h"
#include "../biome/Biome.h"
#include "../biome/BiomeRegistry.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"
#include "../generation/TerrainGenerator.h"
#include "../save/WorldSave.h"

LodGenerator *LodGenerator::GetInstance()
{
    static LodGenerator s_instance;
    return &s_instance;
}

LodGenerator::LodGenerator()
{
    m_pool         = nullptr;
    m_threadCount  = 0;
    m_chunkBacklog = 0;
}

LodGenerator::~LodGenerator()
{
    delete m_pool;
}

void LodGenerator::Submit(std::function<void()> task)
{
    const int coreDivisors[4] = {0, 4, 3, 2};
    int preset                = std::clamp(EversteadServer::GetInstance()->settings.lodCpuPreset, 0, 3);
    int threadCount           = preset == 0 ? 1 : std::max(1, (int) std::thread::hardware_concurrency() / coreDivisors[preset]);

    if (threadCount != m_threadCount)
    {
        delete m_pool;
        m_pool        = new ThreadPool(threadCount, true);
        m_threadCount = threadCount;
    }

    m_pool->DetachTask(std::move(task));
}

void LodGenerator::SetChunkBacklog(int backlog)
{
    m_chunkBacklog = backlog;
}

void LodGenerator::Throttle() const
{
    // We pause per tile instead of waiting for the chunk queue to drain because it never drains while the player keeps moving
    const int pauseMs[4] = {200, 50, 0, 0};
    int pause            = pauseMs[std::clamp(EversteadServer::GetInstance()->settings.lodCpuPreset, 0, 3)];
    if (pause > 0 && m_chunkBacklog > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(pause));
    }
}

LodVolume LodGenerator::BuildTile(uint32_t seed, const WorldGenSettings &settings, int level, int tileX, int tileZ, bool generate)
{
    LodVolume volume = BuildVolume(level, tileX, tileZ, generate);
    if (generate && volume.HasColumnFlag(LodVolume::s_columnMissing) && settings.worldType != eWorldType_Flat)
    {
        Throttle();
        Generate(seed, settings, level, tileX, tileZ, &volume);
        m_store.WriteTile(level, tileX, tileZ, volume);
    }

    return volume;
}

LodVolume LodGenerator::BuildVolume(int level, int tileX, int tileZ, bool keepGenerated)
{
    int sizeY = Chunk::s_sizeY / GetCellHeight(level);
    LodVolume volume;
    if (m_store.ReadTile(level, tileX, tileZ, &volume) && volume.GetSizeX() == s_tileCells && volume.GetSizeY() == sizeY &&
        volume.GetSizeZ() == s_tileCells)
    {
        for (int z = 0; z < s_tileCells && !keepGenerated; z++)
        {
            for (int x = 0; x < s_tileCells; x++)
            {
                if (!(volume.GetFlags(x, z) & LodVolume::s_columnGenerated))
                {
                    continue;
                }

                volume.SetFlags(x, z, LodVolume::s_columnMissing);
                for (int y = 0; y < sizeY; y++)
                {
                    volume.SetBlock(x, y, z, 0);
                }
            }
        }

        return volume;
    }

    volume         = LodVolume(s_tileCells, sizeY, s_tileCells);
    int tileChunks = GetTileSize(level) / Chunk::s_sizeX;
    int minChunkX  = tileX * tileChunks;
    int minChunkZ  = tileZ * tileChunks;
    if (!m_store.HasChunks(minChunkX, minChunkZ, minChunkX + tileChunks - 1, minChunkZ + tileChunks - 1))
    {
        for (int z = 0; z < s_tileCells; z++)
        {
            for (int x = 0; x < s_tileCells; x++)
            {
                volume.SetFlags(x, z, LodVolume::s_columnMissing);
            }
        }

        return volume;
    }

    if (level == 0)
    {
        std::string worldName = m_store.GetWorldName();
        for (int chunkZ = 0; chunkZ < tileChunks; chunkZ++)
        {
            for (int chunkX = 0; chunkX < tileChunks; chunkX++)
            {
                LodVolume chunk;
                bool found = m_store.ReadChunk(minChunkX + chunkX, minChunkZ + chunkZ, &chunk);
                // Chunks saved before LODs were stored on disk only show up once we read them from the world
                if (!found && m_store.TryProbe(minChunkX + chunkX, minChunkZ + chunkZ))
                {
                    Throttle();
                    std::unique_ptr<Chunk> loaded =
                            WorldSave::LoadChunkFromRegion(worldName, eDimensionId_Overworld, minChunkX + chunkX, minChunkZ + chunkZ);
                    if (loaded)
                    {
                        m_store.Ingest(worldName, loaded.get(), false);
                        found = m_store.ReadChunk(minChunkX + chunkX, minChunkZ + chunkZ, &chunk);
                    }
                }

                for (int z = 0; z < Chunk::s_sizeZ; z++)
                {
                    for (int x = 0; x < Chunk::s_sizeX; x++)
                    {
                        int columnX = chunkX * Chunk::s_sizeX + x;
                        int columnZ = chunkZ * Chunk::s_sizeZ + z;
                        if (!found)
                        {
                            volume.SetFlags(columnX, columnZ, LodVolume::s_columnMissing);
                            continue;
                        }

                        volume.SetBiome(columnX, columnZ, chunk.GetBiome(x, z));
                        for (int y = 0; y < sizeY; y++)
                        {
                            volume.SetBlock(columnX, y, columnZ, chunk.GetBlock(x, y, z));
                        }
                    }
                }
            }
        }
    }
    else
    {
        int half = s_tileCells / 2;
        for (int child = 0; child < 4; child++)
        {
            LodVolume source = BuildVolume(level - 1, tileX * 2 + (child & 1), tileZ * 2 + (child >> 1), false);
            int ratio        = source.GetSizeY() / sizeY;
            for (int z = 0; z < half; z++)
            {
                for (int x = 0; x < half; x++)
                {
                    int columnX = (child & 1) * half + x;
                    int columnZ = (child >> 1) * half + z;
                    int present = 0;
                    int columns[4][2];
                    for (int sub = 0; sub < 4; sub++)
                    {
                        int sourceX = x * 2 + (sub & 1);
                        int sourceZ = z * 2 + (sub >> 1);
                        if (!(source.GetFlags(sourceX, sourceZ) & LodVolume::s_columnMissing))
                        {
                            columns[present][0] = sourceX;
                            columns[present][1] = sourceZ;
                            present++;
                        }
                    }

                    if (present == 0)
                    {
                        volume.SetFlags(columnX, columnZ, LodVolume::s_columnMissing);
                        continue;
                    }

                    volume.SetBiome(columnX, columnZ, source.GetBiome(columns[0][0], columns[0][1]));
                    for (int y = 0; y < sizeY; y++)
                    {
                        uint16_t blocks[8];
                        int count = 0;
                        for (int i = 0; i < present; i++)
                        {
                            for (int r = 0; r < ratio; r++)
                            {
                                blocks[count++] = source.GetBlock(columns[i][0], y * ratio + r, columns[i][1]);
                            }
                        }

                        volume.SetBlock(columnX, y, columnZ, LodVolume::PickBlock(blocks, count));
                    }
                }
            }
        }
    }

    m_store.WriteTile(level, tileX, tileZ, volume);
    return volume;
}

void LodGenerator::Generate(uint32_t seed, const WorldGenSettings &settings, int level, int tileX, int tileZ, LodVolume *volume) const
{
    thread_local std::unique_ptr<TerrainGenerator> generator;
    if (!generator || generator->GetSeed() != seed || !(generator->GetSettings() == settings))
    {
        generator = std::make_unique<TerrainGenerator>(seed, settings);
    }

    BiomeRegistry *biomes = BiomeRegistry::GetInstance();
    uint16_t stone        = (uint16_t) Block::ByName("stone")->GetId();
    uint16_t sand         = (uint16_t) Block::ByName("sand")->GetId();
    uint16_t water        = (uint16_t) Block::ByName("water_still")->GetId();
    uint16_t ice          = (uint16_t) Block::ByName("ice")->GetId();
    uint16_t snow         = (uint16_t) Block::ByName("snow_block")->GetId();
    int spacing           = s_minSpacing << level;
    int cellHeight        = GetCellHeight(level);
    int originX           = tileX * s_tileCells * spacing;
    int originZ           = tileZ * s_tileCells * spacing;
    int seaCell           = (TerrainGenerator::s_seaLevel - 1) / cellHeight;
    for (int j = 0; j < s_tileCells; j++)
    {
        for (int i = 0; i < s_tileCells; i++)
        {
            if (!(volume->GetFlags(i, j) & LodVolume::s_columnMissing))
            {
                continue;
            }

            int x      = originX + i * spacing;
            int z      = originZ + j * spacing;
            int height = generator->GetTerrainHeight(x, z);
            // We keep the highest of a few points per cell so hills and cliffs line up with real chunks instead of one point's height
            for (int sub = 1; sub < 4 && spacing > 1; sub++)
            {
                int subX      = originX + i * spacing + (sub & 1) * spacing / 2;
                int subZ      = originZ + j * spacing + (sub >> 1) * spacing / 2;
                int subHeight = generator->GetTerrainHeight(subX, subZ);
                if (subHeight > height)
                {
                    x      = subX;
                    z      = subZ;
                    height = subHeight;
                }
            }

            Biome *biome   = generator->GetBiomeSource().GetBiome(x, z);
            int groundCell = std::clamp(height / cellHeight, 0, volume->GetSizeY() - 1);
            uint16_t top   = (uint16_t) generator->GetSurfaceBlock(biome, x, z, height)->GetId();
            int topCell    = groundCell;
            if (height < TerrainGenerator::s_seaLevel - 1)
            {
                top     = biome->IsSnowyAt(TerrainGenerator::s_seaLevel) ? ice : water;
                topCell = seaCell;
            }
            else if (biome->IsSnowyAt(height))
            {
                top = snow;
            }

            for (int y = 0; y < groundCell; y++)
            {
                volume->SetBlock(i, y, j, stone);
            }

            for (int y = groundCell; y <= topCell; y++)
            {
                volume->SetBlock(i, y, j, y == groundCell && topCell != groundCell ? sand : top);
            }

            int treeSize  = std::max(spacing, 4);
            int clusterX  = Mth::FloorDiv(originX + i * spacing, treeSize);
            int clusterZ  = Mth::FloorDiv(originZ + j * spacing, treeSize);
            uint32_t hash = (uint32_t) clusterX * 73856093 ^ (uint32_t) clusterZ * 19349663 ^ seed;
            hash ^= hash >> 13;
            hash *= 0x5bd1e995;
            hash ^= hash >> 15;
            float coverage = biome->GetTreeDensity() * 0.08f;
            // Cells wider than a tree only show closed canopy since lone trees are too small to see there
            bool tree = topCell == groundCell && (spacing > 4 ? coverage >= 0.5f : (float) (hash & 0xffff) / 65536.0f < coverage);
            if (tree)
            {
                const char *leaves = "oak_leaves";
                switch (biome->GetTreeType())
                {
                    case Biome::eTreeType_SPRUCE:
                    case Biome::eTreeType_MEGA_SPRUCE:
                        leaves = "spruce_leaves";
                        break;
                    case Biome::eTreeType_BIRCH:
                        leaves = "birch_leaves";
                        break;
                    case Biome::eTreeType_ACACIA:
                        leaves = "acacia_leaves";
                        break;
                    case Biome::eTreeType_JUNGLE:
                    case Biome::eTreeType_JUNGLE_EDGE:
                        leaves = "jungle_leaves";
                        break;
                    case Biome::eTreeType_CHERRY:
                        leaves = "cherry_leaves";
                        break;
                    case Biome::eTreeType_DARK_OAK:
                        leaves = "dark_oak_leaves";
                        break;
                    default:
                        break;
                }

                uint16_t leavesId = (uint16_t) Block::ByName(leaves)->GetId();
                int canopyTop     = std::min((height + 5 + (int) (hash >> 16) % 3) / cellHeight, volume->GetSizeY() - 1);
                for (int y = groundCell + 1; y <= canopyTop; y++)
                {
                    volume->SetBlock(i, y, j, leavesId);
                }
            }

            volume->SetBiome(i, j, (uint8_t) biomes->IdOf(biome));
            volume->SetFlags(i, j, LodVolume::s_columnGenerated);
        }
    }

    generator->ClearDensityCache();
}

LodStore *LodGenerator::GetStore()
{
    return &m_store;
}

int LodGenerator::GetTileSize(int level)
{
    return s_tileCells * (s_minSpacing << level);
}

int LodGenerator::GetCellHeight(int level)
{
    return std::min(s_minSpacing << level, s_maxCellHeight);
}
