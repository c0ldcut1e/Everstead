#include "LightEngine.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../core/Logger.h"
#include "../../utils/math/Mth.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"

static const int directions[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

struct LightRemovalNode
{
    int x;
    int y;
    int z;
    uint8_t value;
};

struct ColoredLightRemovalNode
{
    int x;
    int y;
    int z;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

uint8_t LightEngine::Attenuate(uint8_t level, Block *target)
{
    uint8_t cost = (uint8_t) std::max(1, target->GetLightOpacity());
    return level > cost ? level - cost : 0;
}

uint8_t LightEngine::MaxComponent(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t max = r;
    if (g > max)
    {
        max = g;
    }

    if (b > max)
    {
        max = b;
    }

    return max;
}

template<typename T>
struct FastQueue
{
    void Push(const T &v)
    {
        m_data.push_back(v);
    }

    T &Front()
    {
        return m_data[m_head];
    }

    void Pop()
    {
        m_head++;
    }

    bool Empty() const
    {
        return m_head >= m_data.size();
    }

    void Reserve(size_t n)
    {
        m_data.reserve(n);
    }

    void Clear()
    {
        m_data.clear();
        m_head = 0;
    }

    std::vector<T> m_data;
    size_t m_head = 0;
};

struct LightChunkCache
{
    std::unordered_map<ChunkPos, Chunk *, ChunkPosHash> m_map;
    Level *m_level;

    explicit LightChunkCache(Level *level)
    {
        m_level = level;
        m_map.reserve(64);
    }

    Chunk *Get(const ChunkPos &p)
    {
        auto it = m_map.find(p);
        if (it != m_map.end())
        {
            return it->second;
        }

        Chunk *chunk = m_level->GetChunk(p);
        m_map[p]     = chunk;
        return chunk;
    }
};

void LightEngine::GetEmittedLight(Block *block, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t emission = block->GetLightEmission();
    uint8_t lr;
    uint8_t lg;
    uint8_t lb;
    block->GetLightColor(&lr, &lg, &lb);
    *r = (uint8_t) ((lr / 255.0f) * emission);
    *g = (uint8_t) ((lg / 255.0f) * emission);
    *b = (uint8_t) ((lb / 255.0f) * emission);
}

void LightEngine::BuildChunkBlockLight(Chunk *chunk)
{
    static const int directions[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    std::vector<LightNode> queue;
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        if (chunk->IsSectionEmpty(section))
        {
            continue;
        }

        for (int y = section * ChunkSection::s_size; y < (section + 1) * ChunkSection::s_size; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int x = 0; x < Chunk::s_sizeX; x++)
                {
                    Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                    if (block->GetLightEmission() == 0)
                    {
                        continue;
                    }

                    uint8_t r;
                    uint8_t g;
                    uint8_t b;
                    GetEmittedLight(block, &r, &g, &b);
                    chunk->SetBlockLight(x, y, z, r, g, b);
                    queue.push_back({x, y, z, r, g, b});
                }
            }
        }
    }

    for (size_t head = 0; head < queue.size(); head++)
    {
        LightNode node = queue[head];
        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (nx < 0 || nx >= Chunk::s_sizeX || ny < 0 || ny >= Chunk::s_sizeY || nz < 0 || nz >= Chunk::s_sizeZ)
            {
                continue;
            }

            Block *neighborBlock = Block::ById(chunk->GetBlockId(nx, ny, nz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t nr;
            uint8_t ng;
            uint8_t nb;
            chunk->GetBlockLight(nx, ny, nz, &nr, &ng, &nb);
            uint8_t newR = Attenuate(node.r, neighborBlock);
            uint8_t newG = Attenuate(node.g, neighborBlock);
            uint8_t newB = Attenuate(node.b, neighborBlock);
            if (newR > nr || newG > ng || newB > nb)
            {
                uint8_t finalR = std::max(newR, nr);
                uint8_t finalG = std::max(newG, ng);
                uint8_t finalB = std::max(newB, nb);
                chunk->SetBlockLight(nx, ny, nz, finalR, finalG, finalB);
                queue.push_back({nx, ny, nz, finalR, finalG, finalB});
            }
        }
    }
}

int LightEngine::ChunkCoord(int w, int size)
{
    return Mth::FloorDiv(w, size);
}

int LightEngine::LocalCoord(int w, int size)
{
    return Mth::FloorMod(w, size);
}

void LightEngine::RebuildChunk(Level *level, const ChunkPos &pos)
{
    if (!level)
    {
        return;
    }

    Chunk *chunk = level->GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    for (int y = 0; y < Chunk::s_sizeY; y++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                chunk->SetBlockLight(x, y, z, 0, 0, 0);
                chunk->SetSkyLight(x, y, z, 0);
            }
        }
    }

    PropagateSkyLight(level, pos);
    PropagateBlockLight(level, pos);
}

void LightEngine::UpdateChunkSkyLightSeams(Level *level, const ChunkPos &pos)
{
    if (!level)
    {
        return;
    }

    LightChunkCache cache(level);
    Chunk *chunk = cache.Get(pos);
    if (!chunk)
    {
        return;
    }

    std::unordered_set<ChunkPos, ChunkPosHash> dirtyChunks;
    dirtyChunks.reserve(5);

    FastQueue<SkyLightNode> lightQueue;
    lightQueue.Reserve(Chunk::s_sizeY * Chunk::s_sizeX);

    static const int seamOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (int i = 0; i < 4; i++)
    {
        ChunkPos neighborPos(pos.x + seamOffsets[i][0], pos.z + seamOffsets[i][1]);
        Chunk *neighborChunk = cache.Get(neighborPos);
        if (!neighborChunk)
        {
            continue;
        }

        int chunkEdgeX    = -1;
        int chunkEdgeZ    = -1;
        int neighborEdgeX = -1;
        int neighborEdgeZ = -1;
        if (seamOffsets[i][0] < 0)
        {
            chunkEdgeX    = 0;
            neighborEdgeX = Chunk::s_sizeX - 1;
        }
        else if (seamOffsets[i][0] > 0)
        {
            chunkEdgeX    = Chunk::s_sizeX - 1;
            neighborEdgeX = 0;
        }
        else if (seamOffsets[i][1] < 0)
        {
            chunkEdgeZ    = 0;
            neighborEdgeZ = Chunk::s_sizeZ - 1;
        }
        else
        {
            chunkEdgeZ    = Chunk::s_sizeZ - 1;
            neighborEdgeZ = 0;
        }

        if (chunkEdgeX >= 0)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int y = 0; y < Chunk::s_sizeY; y++)
                {
                    uint8_t chunkLevel = chunk->GetSkyLight(chunkEdgeX, y, z);
                    if (chunkLevel > 1)
                    {
                        Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(neighborEdgeX, y, z));
                        if (!neighborBlock->BlocksLight())
                        {
                            uint8_t neighborLevel = neighborChunk->GetSkyLight(neighborEdgeX, y, z);
                            uint8_t newLevel      = Attenuate(chunkLevel, neighborBlock);
                            if (newLevel > neighborLevel)
                            {
                                int wx = neighborPos.x * Chunk::s_sizeX + neighborEdgeX;
                                int wz = neighborPos.z * Chunk::s_sizeZ + z;
                                neighborChunk->SetSkyLight(neighborEdgeX, y, z, newLevel);
                                dirtyChunks.insert(neighborPos);
                                lightQueue.Push({wx, y, wz, newLevel});
                            }
                        }
                    }

                    uint8_t neighborLevel = neighborChunk->GetSkyLight(neighborEdgeX, y, z);
                    if (neighborLevel > 1)
                    {
                        Block *chunkBlock = Block::ById(chunk->GetBlockId(chunkEdgeX, y, z));
                        if (!chunkBlock->BlocksLight())
                        {
                            uint8_t chunkCurrentLevel = chunk->GetSkyLight(chunkEdgeX, y, z);
                            uint8_t newLevel          = Attenuate(neighborLevel, chunkBlock);
                            if (newLevel > chunkCurrentLevel)
                            {
                                int wx = pos.x * Chunk::s_sizeX + chunkEdgeX;
                                int wz = pos.z * Chunk::s_sizeZ + z;
                                chunk->SetSkyLight(chunkEdgeX, y, z, newLevel);
                                dirtyChunks.insert(pos);
                                lightQueue.Push({wx, y, wz, newLevel});
                            }
                        }
                    }
                }
            }
        }
        else
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                for (int y = 0; y < Chunk::s_sizeY; y++)
                {
                    uint8_t chunkLevel = chunk->GetSkyLight(x, y, chunkEdgeZ);
                    if (chunkLevel > 1)
                    {
                        Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(x, y, neighborEdgeZ));
                        if (!neighborBlock->BlocksLight())
                        {
                            uint8_t neighborLevel = neighborChunk->GetSkyLight(x, y, neighborEdgeZ);
                            uint8_t newLevel      = Attenuate(chunkLevel, neighborBlock);
                            if (newLevel > neighborLevel)
                            {
                                int wx = neighborPos.x * Chunk::s_sizeX + x;
                                int wz = neighborPos.z * Chunk::s_sizeZ + neighborEdgeZ;
                                neighborChunk->SetSkyLight(x, y, neighborEdgeZ, newLevel);
                                dirtyChunks.insert(neighborPos);
                                lightQueue.Push({wx, y, wz, newLevel});
                            }
                        }
                    }

                    uint8_t neighborLevel = neighborChunk->GetSkyLight(x, y, neighborEdgeZ);
                    if (neighborLevel > 1)
                    {
                        Block *chunkBlock = Block::ById(chunk->GetBlockId(x, y, chunkEdgeZ));
                        if (!chunkBlock->BlocksLight())
                        {
                            uint8_t chunkCurrentLevel = chunk->GetSkyLight(x, y, chunkEdgeZ);
                            uint8_t newLevel          = Attenuate(neighborLevel, chunkBlock);
                            if (newLevel > chunkCurrentLevel)
                            {
                                int wx = pos.x * Chunk::s_sizeX + x;
                                int wz = pos.z * Chunk::s_sizeZ + chunkEdgeZ;
                                chunk->SetSkyLight(x, y, chunkEdgeZ, newLevel);
                                dirtyChunks.insert(pos);
                                lightQueue.Push({wx, y, wz, newLevel});
                            }
                        }
                    }
                }
            }
        }
    }

    while (!lightQueue.Empty())
    {
        SkyLightNode node = lightQueue.Front();
        lightQueue.Pop();

        int ncx0         = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0         = ChunkCoord(node.z, Chunk::s_sizeZ);
        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        int lx0 = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0 = LocalCoord(node.z, Chunk::s_sizeZ);

        uint8_t currentLevel = nodeChunk->GetSkyLight(lx0, node.y, lz0);
        if (currentLevel <= 1)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);
            ChunkPos neighborChunkPos(ncx, ncz);
            Chunk *neighborChunk = cache.Get(neighborChunkPos);
            if (!neighborChunk)
            {
                continue;
            }

            int lx = LocalCoord(nx, Chunk::s_sizeX);
            int lz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(lx, ny, lz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t neighborLevel = neighborChunk->GetSkyLight(lx, ny, lz);

            uint8_t newLevel;
            if (directions[i][1] == -1 && currentLevel == 15 && neighborBlock->GetLightOpacity() == 0)
            {
                newLevel = 15;
            }
            else
            {
                newLevel = Attenuate(currentLevel, neighborBlock);
            }

            if (newLevel > neighborLevel)
            {
                neighborChunk->SetSkyLight(lx, ny, lz, newLevel);
                dirtyChunks.insert(neighborChunkPos);
                lightQueue.Push({nx, ny, nz, newLevel});
            }
        }
    }

    for (const ChunkPos &dirtyChunk : dirtyChunks)
    {
        level->MarkChunkDirty(BlockPos(dirtyChunk.x * Chunk::s_sizeX, 0, dirtyChunk.z * Chunk::s_sizeZ));
    }
}

void LightEngine::PropagateSkyLight(Level *level, const ChunkPos &pos)
{
    Chunk *chunk = level->GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    LightChunkCache cache(level);
    cache.m_map[pos] = chunk;

    FastQueue<SkyLightNode> lightQueue;
    lightQueue.Reserve(Chunk::s_sizeX * Chunk::s_sizeZ * 16);

    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            int wx = pos.x * Chunk::s_sizeX + x;
            int wz = pos.z * Chunk::s_sizeZ + z;

            for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
            {
                Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                if (block->BlocksLight() || block->GetLightOpacity() > 0)
                {
                    break;
                }

                chunk->SetSkyLight(x, y, z, 15);
                lightQueue.Push({wx, y, wz, 15});
            }
        }
    }

    static const int neighborOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (int n = 0; n < 4; n++)
    {
        int ncx = pos.x + neighborOffsets[n][0];
        int ncz = pos.z + neighborOffsets[n][1];

        Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
        if (!neighborChunk)
        {
            continue;
        }

        int edgeX  = neighborOffsets[n][0] == -1 ? 0 : (neighborOffsets[n][0] == 1 ? Chunk::s_sizeX - 1 : -1);
        int edgeZ  = neighborOffsets[n][1] == -1 ? 0 : (neighborOffsets[n][1] == 1 ? Chunk::s_sizeZ - 1 : -1);
        int nEdgeX = neighborOffsets[n][0] == -1 ? Chunk::s_sizeX - 1 : (neighborOffsets[n][0] == 1 ? 0 : -1);
        int nEdgeZ = neighborOffsets[n][1] == -1 ? Chunk::s_sizeZ - 1 : (neighborOffsets[n][1] == 1 ? 0 : -1);

        if (edgeX >= 0)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int y = 0; y < Chunk::s_sizeY; y++)
                {
                    uint8_t neighborLevel = neighborChunk->GetSkyLight(nEdgeX, y, z);
                    if (neighborLevel <= 1)
                    {
                        continue;
                    }

                    Block *block = Block::ById(chunk->GetBlockId(edgeX, y, z));
                    if (block->BlocksLight())
                    {
                        continue;
                    }

                    uint8_t newLevel = Attenuate(neighborLevel, block);
                    if (newLevel > chunk->GetSkyLight(edgeX, y, z))
                    {
                        int wx = pos.x * Chunk::s_sizeX + edgeX;
                        int wz = pos.z * Chunk::s_sizeZ + z;
                        chunk->SetSkyLight(edgeX, y, z, newLevel);
                        lightQueue.Push({wx, y, wz, newLevel});
                    }
                }
            }
        }
        else
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                for (int y = 0; y < Chunk::s_sizeY; y++)
                {
                    uint8_t neighborLevel = neighborChunk->GetSkyLight(x, y, nEdgeZ);
                    if (neighborLevel <= 1)
                    {
                        continue;
                    }

                    Block *block = Block::ById(chunk->GetBlockId(x, y, edgeZ));
                    if (block->BlocksLight())
                    {
                        continue;
                    }

                    uint8_t newLevel = Attenuate(neighborLevel, block);
                    if (newLevel > chunk->GetSkyLight(x, y, edgeZ))
                    {
                        int wx = pos.x * Chunk::s_sizeX + x;
                        int wz = pos.z * Chunk::s_sizeZ + edgeZ;
                        chunk->SetSkyLight(x, y, edgeZ, newLevel);
                        lightQueue.Push({wx, y, wz, newLevel});
                    }
                }
            }
        }
    }

    while (!lightQueue.Empty())
    {
        SkyLightNode node = lightQueue.Front();
        lightQueue.Pop();

        int ncx0 = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0 = ChunkCoord(node.z, Chunk::s_sizeZ);
        int lx0  = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0  = LocalCoord(node.z, Chunk::s_sizeZ);

        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        uint8_t currentLevel = nodeChunk->GetSkyLight(lx0, node.y, lz0);
        if (currentLevel == 0)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];

            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int lx = LocalCoord(nx, Chunk::s_sizeX);
            int lz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(lx, ny, lz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t neighborLevel = neighborChunk->GetSkyLight(lx, ny, lz);

            uint8_t newLevel;
            if (directions[i][1] == -1 && currentLevel == 15 && neighborBlock->GetLightOpacity() == 0)
            {
                newLevel = 15;
            }
            else
            {
                newLevel = Attenuate(currentLevel, neighborBlock);
            }

            if (newLevel > neighborLevel)
            {
                neighborChunk->SetSkyLight(lx, ny, lz, newLevel);
                lightQueue.Push({nx, ny, nz, newLevel});
            }
        }
    }
}

void LightEngine::PropagateBlockLight(Level *level, const ChunkPos &pos)
{
    Chunk *chunk = level->GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    LightChunkCache cache(level);
    cache.m_map[pos] = chunk;

    FastQueue<LightNode> lightQueue;
    lightQueue.Reserve(Chunk::s_sizeX * Chunk::s_sizeZ * 16);

    std::unordered_set<ChunkPos, ChunkPosHash> dirtyChunks;
    for (int y = 0; y < Chunk::s_sizeY; y++)
    {
        if (chunk->IsSectionEmpty(y / ChunkSection::s_size))
        {
            y += ChunkSection::s_size - 1;
            continue;
        }

        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                if (block->GetLightEmission() == 0)
                {
                    continue;
                }

                uint8_t finalR;
                uint8_t finalG;
                uint8_t finalB;
                GetEmittedLight(block, &finalR, &finalG, &finalB);

                int wx = pos.x * Chunk::s_sizeX + x;
                int wz = pos.z * Chunk::s_sizeZ + z;

                uint8_t oldR;
                uint8_t oldG;
                uint8_t oldB;
                chunk->GetBlockLight(x, y, z, &oldR, &oldG, &oldB);
                if (oldR != finalR || oldG != finalG || oldB != finalB)
                {
                    chunk->SetBlockLight(x, y, z, finalR, finalG, finalB);
                    dirtyChunks.insert(pos);
                }
                lightQueue.Push({wx, y, wz, finalR, finalG, finalB});
            }
        }
    }

    static const int neighborOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (int n = 0; n < 4; n++)
    {
        int ncx = pos.x + neighborOffsets[n][0];
        int ncz = pos.z + neighborOffsets[n][1];

        Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
        if (!neighborChunk)
        {
            continue;
        }

        int edgeX  = neighborOffsets[n][0] == -1 ? 0 : (neighborOffsets[n][0] == 1 ? Chunk::s_sizeX - 1 : -1);
        int edgeZ  = neighborOffsets[n][1] == -1 ? 0 : (neighborOffsets[n][1] == 1 ? Chunk::s_sizeZ - 1 : -1);
        int nEdgeX = neighborOffsets[n][0] == -1 ? Chunk::s_sizeX - 1 : (neighborOffsets[n][0] == 1 ? 0 : -1);
        int nEdgeZ = neighborOffsets[n][1] == -1 ? Chunk::s_sizeZ - 1 : (neighborOffsets[n][1] == 1 ? 0 : -1);

        if (edgeX >= 0)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int y = 0; y < Chunk::s_sizeY; y++)
                {
                    uint8_t er, eg, eb;
                    chunk->GetBlockLight(edgeX, y, z, &er, &eg, &eb);
                    if (MaxComponent(er, eg, eb) > 1)
                    {
                        lightQueue.Push({pos.x * Chunk::s_sizeX + edgeX, y, pos.z * Chunk::s_sizeZ + z, er, eg, eb});
                    }

                    uint8_t nr, ng, nb;
                    neighborChunk->GetBlockLight(nEdgeX, y, z, &nr, &ng, &nb);
                    if (MaxComponent(nr, ng, nb) <= 1)
                    {
                        continue;
                    }

                    Block *block = Block::ById(chunk->GetBlockId(edgeX, y, z));
                    if (block->BlocksLight())
                    {
                        continue;
                    }

                    uint8_t newR = Attenuate(nr, block);
                    uint8_t newG = Attenuate(ng, block);
                    uint8_t newB = Attenuate(nb, block);

                    uint8_t cr, cg, cb;
                    chunk->GetBlockLight(edgeX, y, z, &cr, &cg, &cb);

                    if (newR > cr || newG > cg || newB > cb)
                    {
                        uint8_t finalR = newR > cr ? newR : cr;
                        uint8_t finalG = newG > cg ? newG : cg;
                        uint8_t finalB = newB > cb ? newB : cb;

                        int wx = pos.x * Chunk::s_sizeX + edgeX;
                        int wz = pos.z * Chunk::s_sizeZ + z;
                        chunk->SetBlockLight(edgeX, y, z, finalR, finalG, finalB);
                        lightQueue.Push({wx, y, wz, finalR, finalG, finalB});
                        dirtyChunks.insert(pos);
                    }
                }
            }
        }
        else
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                for (int y = 0; y < Chunk::s_sizeY; y++)
                {
                    uint8_t er, eg, eb;
                    chunk->GetBlockLight(x, y, edgeZ, &er, &eg, &eb);
                    if (MaxComponent(er, eg, eb) > 1)
                    {
                        lightQueue.Push({pos.x * Chunk::s_sizeX + x, y, pos.z * Chunk::s_sizeZ + edgeZ, er, eg, eb});
                    }

                    uint8_t nr, ng, nb;
                    neighborChunk->GetBlockLight(x, y, nEdgeZ, &nr, &ng, &nb);
                    if (MaxComponent(nr, ng, nb) <= 1)
                    {
                        continue;
                    }

                    Block *block = Block::ById(chunk->GetBlockId(x, y, edgeZ));
                    if (block->BlocksLight())
                    {
                        continue;
                    }

                    uint8_t newR = Attenuate(nr, block);
                    uint8_t newG = Attenuate(ng, block);
                    uint8_t newB = Attenuate(nb, block);

                    uint8_t cr, cg, cb;
                    chunk->GetBlockLight(x, y, edgeZ, &cr, &cg, &cb);

                    if (newR > cr || newG > cg || newB > cb)
                    {
                        uint8_t finalR = newR > cr ? newR : cr;
                        uint8_t finalG = newG > cg ? newG : cg;
                        uint8_t finalB = newB > cb ? newB : cb;

                        int wx = pos.x * Chunk::s_sizeX + x;
                        int wz = pos.z * Chunk::s_sizeZ + edgeZ;
                        chunk->SetBlockLight(x, y, edgeZ, finalR, finalG, finalB);
                        lightQueue.Push({wx, y, wz, finalR, finalG, finalB});
                        dirtyChunks.insert(pos);
                    }
                }
            }
        }
    }

    while (!lightQueue.Empty())
    {
        LightNode node = lightQueue.Front();
        lightQueue.Pop();

        int ncx0 = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0 = ChunkCoord(node.z, Chunk::s_sizeZ);
        int lx0  = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0  = LocalCoord(node.z, Chunk::s_sizeZ);

        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        uint8_t cr, cg, cb;
        nodeChunk->GetBlockLight(lx0, node.y, lz0, &cr, &cg, &cb);

        uint8_t maxCurrent = MaxComponent(cr, cg, cb);
        if (maxCurrent <= 1)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];

            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int lx = LocalCoord(nx, Chunk::s_sizeX);
            int lz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(lx, ny, lz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t nr, ng, nb;
            neighborChunk->GetBlockLight(lx, ny, lz, &nr, &ng, &nb);

            uint8_t newR = Attenuate(cr, neighborBlock);
            uint8_t newG = Attenuate(cg, neighborBlock);
            uint8_t newB = Attenuate(cb, neighborBlock);
            if (newR > nr || newG > ng || newB > nb)
            {
                uint8_t finalR = newR > nr ? newR : nr;
                uint8_t finalG = newG > ng ? newG : ng;
                uint8_t finalB = newB > nb ? newB : nb;

                neighborChunk->SetBlockLight(lx, ny, lz, finalR, finalG, finalB);
                lightQueue.Push({nx, ny, nz, finalR, finalG, finalB});
                dirtyChunks.insert(ChunkPos(ncx, ncz));
            }
        }
    }

    for (const ChunkPos &dirtyChunk : dirtyChunks)
    {
        level->MarkChunkDirty(BlockPos(dirtyChunk.x * Chunk::s_sizeX, 0, dirtyChunk.z * Chunk::s_sizeZ));
    }
}

void LightEngine::Rebuild(Level *level)
{
    if (!level)
    {
        return;
    }

    std::vector<std::pair<ChunkPos, Chunk *>> chunks = level->GetLoadedChunks();

    for (auto chunkIt = chunks.begin(); chunkIt != chunks.end(); ++chunkIt)
    {
        Chunk *chunk = chunkIt->second;
        if (!chunk)
        {
            continue;
        }

        for (int y = 0; y < Chunk::s_sizeY; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int x = 0; x < Chunk::s_sizeX; x++)
                {
                    chunk->SetBlockLight(x, y, z, 0, 0, 0);
                    chunk->SetSkyLight(x, y, z, 0);
                }
            }
        }
    }

    LightChunkCache cache(level);
    FastQueue<SkyLightNode> skyQueue;
    skyQueue.Reserve(chunks.size() * Chunk::s_sizeX * Chunk::s_sizeZ * 4);

    for (auto chunkIt = chunks.begin(); chunkIt != chunks.end(); ++chunkIt)
    {
        const ChunkPos &pos = chunkIt->first;
        Chunk *chunk        = chunkIt->second;
        if (!chunk)
        {
            continue;
        }

        cache.m_map[ChunkPos(pos.x, pos.z)] = chunk;

        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                int wx = pos.x * Chunk::s_sizeX + x;
                int wz = pos.z * Chunk::s_sizeZ + z;

                for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
                {
                    Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                    if (block->BlocksLight() || block->GetLightOpacity() > 0)
                    {
                        break;
                    }

                    chunk->SetSkyLight(x, y, z, 15);
                    skyQueue.Push({wx, y, wz, 15});
                }
            }
        }
    }

    while (!skyQueue.Empty())
    {
        SkyLightNode node = skyQueue.Front();
        skyQueue.Pop();

        int ncx0 = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0 = ChunkCoord(node.z, Chunk::s_sizeZ);
        int lx0  = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0  = LocalCoord(node.z, Chunk::s_sizeZ);

        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        uint8_t currentLevel = nodeChunk->GetSkyLight(lx0, node.y, lz0);
        if (currentLevel == 0)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];

            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int lx = LocalCoord(nx, Chunk::s_sizeX);
            int lz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(lx, ny, lz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t neighborLevel = neighborChunk->GetSkyLight(lx, ny, lz);

            uint8_t newLevel;
            if (directions[i][1] == -1 && currentLevel == 15 && neighborBlock->GetLightOpacity() == 0)
            {
                newLevel = 15;
            }
            else
            {
                newLevel = Attenuate(currentLevel, neighborBlock);
            }

            if (newLevel > neighborLevel)
            {
                neighborChunk->SetSkyLight(lx, ny, lz, newLevel);
                skyQueue.Push({nx, ny, nz, newLevel});
            }
        }
    }

    FastQueue<LightNode> blockQueue;
    blockQueue.Reserve(chunks.size() * 16);

    for (auto chunkIt = chunks.begin(); chunkIt != chunks.end(); ++chunkIt)
    {
        const ChunkPos &pos = chunkIt->first;
        Chunk *chunk        = chunkIt->second;
        if (!chunk)
        {
            continue;
        }

        for (int y = 0; y < Chunk::s_sizeY; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                for (int x = 0; x < Chunk::s_sizeX; x++)
                {
                    Block *block     = Block::ById(chunk->GetBlockId(x, y, z));
                    uint8_t emission = block->GetLightEmission();
                    if (emission == 0)
                    {
                        continue;
                    }

                    uint8_t lr, lg, lb;
                    block->GetLightColor(&lr, &lg, &lb);

                    uint8_t finalR = (uint8_t) ((lr / 255.0f) * emission);
                    uint8_t finalG = (uint8_t) ((lg / 255.0f) * emission);
                    uint8_t finalB = (uint8_t) ((lb / 255.0f) * emission);

                    int wx = pos.x * Chunk::s_sizeX + x;
                    int wz = pos.z * Chunk::s_sizeZ + z;

                    chunk->SetBlockLight(x, y, z, finalR, finalG, finalB);
                    blockQueue.Push({wx, y, wz, finalR, finalG, finalB});
                }
            }
        }
    }

    while (!blockQueue.Empty())
    {
        LightNode node = blockQueue.Front();
        blockQueue.Pop();

        int ncx0 = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0 = ChunkCoord(node.z, Chunk::s_sizeZ);
        int lx0  = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0  = LocalCoord(node.z, Chunk::s_sizeZ);

        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        uint8_t cr, cg, cb;
        nodeChunk->GetBlockLight(lx0, node.y, lz0, &cr, &cg, &cb);

        uint8_t maxCurrent = MaxComponent(cr, cg, cb);
        if (maxCurrent <= 1)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];

            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int lx = LocalCoord(nx, Chunk::s_sizeX);
            int lz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(lx, ny, lz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t nr, ng, nb;
            neighborChunk->GetBlockLight(lx, ny, lz, &nr, &ng, &nb);

            uint8_t newR = Attenuate(cr, neighborBlock);
            uint8_t newG = Attenuate(cg, neighborBlock);
            uint8_t newB = Attenuate(cb, neighborBlock);
            if (newR > nr || newG > ng || newB > nb)
            {
                uint8_t finalR = newR > nr ? newR : nr;
                uint8_t finalG = newG > ng ? newG : ng;
                uint8_t finalB = newB > nb ? newB : nb;

                neighborChunk->SetBlockLight(lx, ny, lz, finalR, finalG, finalB);
                blockQueue.Push({nx, ny, nz, finalR, finalG, finalB});
            }
        }
    }
}

void LightEngine::UpdateFrom(Level *level, const BlockPos &levelPos)
{
    if (!level)
    {
        return;
    }

    if (levelPos.y < 0 || levelPos.y >= Chunk::s_sizeY)
    {
        return;
    }

    int cx = ChunkCoord(levelPos.x, Chunk::s_sizeX);
    int cz = ChunkCoord(levelPos.z, Chunk::s_sizeZ);

    int lx = LocalCoord(levelPos.x, Chunk::s_sizeX);
    int lz = LocalCoord(levelPos.z, Chunk::s_sizeZ);

    LightChunkCache cache(level);

    Chunk *baseChunk = cache.Get(ChunkPos(cx, cz));
    if (!baseChunk)
    {
        return;
    }

    Block *changedBlock = Block::ById(baseChunk->GetBlockId(lx, levelPos.y, lz));

    uint8_t oldR = 0;
    uint8_t oldG = 0;
    uint8_t oldB = 0;
    baseChunk->GetBlockLight(lx, levelPos.y, lz, &oldR, &oldG, &oldB);

    std::unordered_set<SectionPos, SectionPosHash> rebuildSections;
    auto MarkTouched = [&](int x, int y, int z)
    {
        MarkLightNeighborhood(x, y, z, &rebuildSections);
    };

    FastQueue<LightRemovalNode> skyRemovalQueue;
    FastQueue<SkyLightNode> skyAddQueue;
    FastQueue<ColoredLightRemovalNode> blockRemovalQueue;
    FastQueue<LightNode> blockAddQueue;

    skyRemovalQueue.Reserve(256);
    skyAddQueue.Reserve(256);
    blockRemovalQueue.Reserve(256);
    blockAddQueue.Reserve(256);

    uint8_t oldSkyLevel = baseChunk->GetSkyLight(lx, levelPos.y, lz);
    if (oldSkyLevel > 0 && (changedBlock->BlocksLight() || changedBlock->GetLightOpacity() > 0))
    {
        MarkTouched(levelPos.x, levelPos.y, levelPos.z);
        baseChunk->SetSkyLight(lx, levelPos.y, lz, 0);
        skyRemovalQueue.Push({levelPos.x, levelPos.y, levelPos.z, oldSkyLevel});
    }

    while (!skyRemovalQueue.Empty())
    {
        LightRemovalNode node = skyRemovalQueue.Front();
        skyRemovalQueue.Pop();

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int nlx = LocalCoord(nx, Chunk::s_sizeX);
            int nlz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(nlx, ny, nlz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t neighborLevel = neighborChunk->GetSkyLight(nlx, ny, nlz);
            if (neighborLevel == 0)
            {
                continue;
            }

            uint8_t expectedFromThis;
            if (directions[i][1] == -1 && node.value == 15 && neighborBlock->GetLightOpacity() == 0)
            {
                expectedFromThis = 15;
            }
            else
            {
                expectedFromThis = Attenuate(node.value, neighborBlock);
            }

            if (neighborLevel <= expectedFromThis)
            {
                MarkTouched(nx, ny, nz);
                neighborChunk->SetSkyLight(nlx, ny, nlz, 0);
                skyRemovalQueue.Push({nx, ny, nz, neighborLevel});
            }
            else
            {
                skyAddQueue.Push({nx, ny, nz, neighborLevel});
            }
        }
    }

    for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
    {
        Block *block = Block::ById(baseChunk->GetBlockId(lx, y, lz));
        if (block->BlocksLight() || block->GetLightOpacity() > 0)
        {
            break;
        }

        if (baseChunk->GetSkyLight(lx, y, lz) < 15)
        {
            MarkTouched(levelPos.x, y, levelPos.z);
            baseChunk->SetSkyLight(lx, y, lz, 15);
            skyAddQueue.Push({levelPos.x, y, levelPos.z, 15});
        }
    }

    for (int i = 0; i < 6; i++)
    {
        int nx = levelPos.x + directions[i][0];
        int ny = levelPos.y + directions[i][1];
        int nz = levelPos.z + directions[i][2];

        if (ny < 0 || ny >= Chunk::s_sizeY)
        {
            continue;
        }

        int ncx = ChunkCoord(nx, Chunk::s_sizeX);
        int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

        Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
        if (!neighborChunk)
        {
            continue;
        }

        int nlx = LocalCoord(nx, Chunk::s_sizeX);
        int nlz = LocalCoord(nz, Chunk::s_sizeZ);

        uint8_t neighborLevel = neighborChunk->GetSkyLight(nlx, ny, nlz);
        if (neighborLevel > 0)
        {
            skyAddQueue.Push({nx, ny, nz, neighborLevel});
        }
    }

    while (!skyAddQueue.Empty())
    {
        SkyLightNode node = skyAddQueue.Front();
        skyAddQueue.Pop();

        int ncx0         = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0         = ChunkCoord(node.z, Chunk::s_sizeZ);
        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        int lx0 = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0 = LocalCoord(node.z, Chunk::s_sizeZ);

        uint8_t currentLevel = nodeChunk->GetSkyLight(lx0, node.y, lz0);
        if (currentLevel == 0)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int nlx = LocalCoord(nx, Chunk::s_sizeX);
            int nlz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(nlx, ny, nlz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t neighborLevel = neighborChunk->GetSkyLight(nlx, ny, nlz);

            uint8_t newLevel;
            if (directions[i][1] == -1 && currentLevel == 15 && neighborBlock->GetLightOpacity() == 0)
            {
                newLevel = 15;
            }
            else
            {
                newLevel = Attenuate(currentLevel, neighborBlock);
            }

            if (newLevel > neighborLevel)
            {
                MarkTouched(nx, ny, nz);
                neighborChunk->SetSkyLight(nlx, ny, nlz, newLevel);
                skyAddQueue.Push({nx, ny, nz, newLevel});
            }
        }
    }

    uint8_t emittedR = 0;
    uint8_t emittedG = 0;
    uint8_t emittedB = 0;
    GetEmittedLight(changedBlock, &emittedR, &emittedG, &emittedB);
    bool onlyBrighter = emittedR >= oldR && emittedG >= oldG && emittedB >= oldB;
    if ((oldR > 0 || oldG > 0 || oldB > 0) && !onlyBrighter)
    {
        MarkTouched(levelPos.x, levelPos.y, levelPos.z);
        baseChunk->SetBlockLight(lx, levelPos.y, lz, 0, 0, 0);
        blockRemovalQueue.Push({levelPos.x, levelPos.y, levelPos.z, oldR, oldG, oldB});
    }

    while (!blockRemovalQueue.Empty())
    {
        ColoredLightRemovalNode node = blockRemovalQueue.Front();
        blockRemovalQueue.Pop();

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int nlx              = LocalCoord(nx, Chunk::s_sizeX);
            int nlz              = LocalCoord(nz, Chunk::s_sizeZ);
            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(nlx, ny, nlz));
            bool emitter         = neighborBlock->GetLightEmission() > 0;
            if (neighborBlock->BlocksLight() && !emitter)
            {
                continue;
            }

            uint8_t neighborLight[3];
            neighborChunk->GetBlockLight(nlx, ny, nlz, &neighborLight[0], &neighborLight[1], &neighborLight[2]);

            uint8_t nodeLight[3]    = {node.r, node.g, node.b};
            uint8_t removedLight[3] = {0, 0, 0};
            bool removed            = false;
            bool relight            = false;
            for (int channel = 0; channel < 3; channel++)
            {
                if (nodeLight[channel] == 0 || neighborLight[channel] == 0)
                {
                    continue;
                }

                if (neighborLight[channel] < nodeLight[channel] && !neighborBlock->BlocksLight())
                {
                    removedLight[channel]  = neighborLight[channel];
                    neighborLight[channel] = 0;
                    removed                = true;
                }
                else
                {
                    relight = true;
                }
            }

            if (removed)
            {
                blockRemovalQueue.Push({nx, ny, nz, removedLight[0], removedLight[1], removedLight[2]});
            }

            if (emitter)
            {
                uint8_t emitted[3];
                GetEmittedLight(neighborBlock, &emitted[0], &emitted[1], &emitted[2]);
                for (int channel = 0; channel < 3; channel++)
                {
                    neighborLight[channel] = std::max(neighborLight[channel], emitted[channel]);
                }

                relight = true;
            }

            if (removed || emitter)
            {
                MarkTouched(nx, ny, nz);
                neighborChunk->SetBlockLight(nlx, ny, nlz, neighborLight[0], neighborLight[1], neighborLight[2]);
            }

            if (relight)
            {
                blockAddQueue.Push({nx, ny, nz, neighborLight[0], neighborLight[1], neighborLight[2]});
            }
        }
    }

    if (changedBlock->GetLightEmission() > 0)
    {
        uint8_t finalR;
        uint8_t finalG;
        uint8_t finalB;
        GetEmittedLight(changedBlock, &finalR, &finalG, &finalB);
        MarkTouched(levelPos.x, levelPos.y, levelPos.z);
        baseChunk->SetBlockLight(lx, levelPos.y, lz, finalR, finalG, finalB);
        blockAddQueue.Push({levelPos.x, levelPos.y, levelPos.z, finalR, finalG, finalB});
    }

    if (!changedBlock->BlocksLight())
    {
        for (int i = 0; i < 6; i++)
        {
            int nx = levelPos.x + directions[i][0];
            int ny = levelPos.y + directions[i][1];
            int nz = levelPos.z + directions[i][2];
            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int nlx = LocalCoord(nx, Chunk::s_sizeX);
            int nlz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(nlx, ny, nlz));
            if (neighborBlock->BlocksLight() && neighborBlock->GetLightEmission() == 0)
            {
                continue;
            }

            uint8_t nr;
            uint8_t ng;
            uint8_t nb;
            neighborChunk->GetBlockLight(nlx, ny, nlz, &nr, &ng, &nb);
            if (nr || ng || nb)
            {
                blockAddQueue.Push({nx, ny, nz, nr, ng, nb});
            }
        }
    }

    while (!blockAddQueue.Empty())
    {
        LightNode node = blockAddQueue.Front();
        blockAddQueue.Pop();

        int ncx0         = ChunkCoord(node.x, Chunk::s_sizeX);
        int ncz0         = ChunkCoord(node.z, Chunk::s_sizeZ);
        Chunk *nodeChunk = cache.Get(ChunkPos(ncx0, ncz0));
        if (!nodeChunk)
        {
            continue;
        }

        int lx0 = LocalCoord(node.x, Chunk::s_sizeX);
        int lz0 = LocalCoord(node.z, Chunk::s_sizeZ);

        uint8_t cr, cg, cb;
        nodeChunk->GetBlockLight(lx0, node.y, lz0, &cr, &cg, &cb);

        uint8_t maxCurrent = MaxComponent(cr, cg, cb);
        if (maxCurrent <= 1)
        {
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            int nx = node.x + directions[i][0];
            int ny = node.y + directions[i][1];
            int nz = node.z + directions[i][2];
            if (ny < 0 || ny >= Chunk::s_sizeY)
            {
                continue;
            }

            int ncx = ChunkCoord(nx, Chunk::s_sizeX);
            int ncz = ChunkCoord(nz, Chunk::s_sizeZ);

            Chunk *neighborChunk = cache.Get(ChunkPos(ncx, ncz));
            if (!neighborChunk)
            {
                continue;
            }

            int nlx = LocalCoord(nx, Chunk::s_sizeX);
            int nlz = LocalCoord(nz, Chunk::s_sizeZ);

            Block *neighborBlock = Block::ById(neighborChunk->GetBlockId(nlx, ny, nlz));
            if (neighborBlock->BlocksLight())
            {
                continue;
            }

            uint8_t nr;
            uint8_t ng;
            uint8_t nb;
            neighborChunk->GetBlockLight(nlx, ny, nlz, &nr, &ng, &nb);

            uint8_t newR = Attenuate(cr, neighborBlock);
            uint8_t newG = Attenuate(cg, neighborBlock);
            uint8_t newB = Attenuate(cb, neighborBlock);

            if (newR > nr || newG > ng || newB > nb)
            {
                uint8_t finalR = newR > nr ? newR : nr;
                uint8_t finalG = newG > ng ? newG : ng;
                uint8_t finalB = newB > nb ? newB : nb;

                MarkTouched(nx, ny, nz);
                neighborChunk->SetBlockLight(nlx, ny, nlz, finalR, finalG, finalB);
                blockAddQueue.Push({nx, ny, nz, finalR, finalG, finalB});
            }
        }
    }

    if (!level->IsClientSide())
    {
        return;
    }

    MarkLightNeighborhood(levelPos.x, levelPos.y, levelPos.z, &rebuildSections);
    level->OnBlockLightChanged(rebuildSections, !Block::ById(level->GetBlockId(levelPos))->IsLiquid());
}

void LightEngine::MarkLightNeighborhood(int x, int y, int z, std::unordered_set<SectionPos, SectionPosHash> *sections)
{
    for (int dy = -1; dy <= 1; dy += 2)
    {
        int sectionY = std::clamp(y + dy, 0, Chunk::s_sizeY - 1) / ChunkSection::s_size;
        for (int dz = -1; dz <= 1; dz += 2)
        {
            for (int dx = -1; dx <= 1; dx += 2)
            {
                sections->insert(SectionPos(ChunkCoord(x + dx, Chunk::s_sizeX), sectionY, ChunkCoord(z + dz, Chunk::s_sizeZ)));
            }
        }
    }
}

void LightEngine::GetBlockLight(Level *level, const BlockPos &levelPos, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (levelPos.y < 0 || levelPos.y >= Chunk::s_sizeY)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    int cx = ChunkCoord(levelPos.x, Chunk::s_sizeX);
    int cz = ChunkCoord(levelPos.z, Chunk::s_sizeZ);

    ChunkPos chunkPos(cx, cz);
    Chunk *chunk = level->GetChunk(chunkPos);
    if (!chunk)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    int lx = LocalCoord(levelPos.x, Chunk::s_sizeX);
    int lz = LocalCoord(levelPos.z, Chunk::s_sizeZ);
    chunk->GetBlockLight(lx, levelPos.y, lz, r, g, b);
}

void LightEngine::SetBlockLight(Level *level, const BlockPos &levelPos, uint8_t r, uint8_t g, uint8_t b)
{
    if (levelPos.y < 0 || levelPos.y >= Chunk::s_sizeY)
    {
        return;
    }

    int cx = ChunkCoord(levelPos.x, Chunk::s_sizeX);
    int cz = ChunkCoord(levelPos.z, Chunk::s_sizeZ);

    ChunkPos chunkPos(cx, cz);
    Chunk *chunk = level->GetChunk(chunkPos);
    if (!chunk)
    {
        return;
    }

    int lx = LocalCoord(levelPos.x, Chunk::s_sizeX);
    int lz = LocalCoord(levelPos.z, Chunk::s_sizeZ);
    chunk->SetBlockLight(lx, levelPos.y, lz, r, g, b);
}

uint8_t LightEngine::GetSkyLight(Level *level, const BlockPos &levelPos)
{
    if (levelPos.y < 0 || levelPos.y >= Chunk::s_sizeY)
    {
        return 0;
    }

    int cx = ChunkCoord(levelPos.x, Chunk::s_sizeX);
    int cz = ChunkCoord(levelPos.z, Chunk::s_sizeZ);

    ChunkPos chunkPos(cx, cz);
    Chunk *chunk = level->GetChunk(chunkPos);
    if (!chunk)
    {
        return 0;
    }

    int lx = LocalCoord(levelPos.x, Chunk::s_sizeX);
    int lz = LocalCoord(levelPos.z, Chunk::s_sizeZ);
    return chunk->GetSkyLight(lx, levelPos.y, lz);
}

void LightEngine::SetSkyLight(Level *level, const BlockPos &levelPos, uint8_t lightLevel)
{
    if (levelPos.y < 0 || levelPos.y >= Chunk::s_sizeY)
    {
        return;
    }

    int cx = ChunkCoord(levelPos.x, Chunk::s_sizeX);
    int cz = ChunkCoord(levelPos.z, Chunk::s_sizeZ);
    ChunkPos chunkPos(cx, cz);
    Chunk *chunk = level->GetChunk(chunkPos);
    if (!chunk)
    {
        return;
    }

    int lx = LocalCoord(levelPos.x, Chunk::s_sizeX);
    int lz = LocalCoord(levelPos.z, Chunk::s_sizeZ);
    chunk->SetSkyLight(lx, levelPos.y, lz, lightLevel);
}

void LightEngine::GetLightLevel(Level *level, const BlockPos &levelPos, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (levelPos.y < 0 || levelPos.y >= Chunk::s_sizeY)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    int cx = ChunkCoord(levelPos.x, Chunk::s_sizeX);
    int cz = ChunkCoord(levelPos.z, Chunk::s_sizeZ);

    ChunkPos chunkPos(cx, cz);
    Chunk *chunk = level->GetChunk(chunkPos);
    if (!chunk)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    int lx = LocalCoord(levelPos.x, Chunk::s_sizeX);
    int lz = LocalCoord(levelPos.z, Chunk::s_sizeZ);
    chunk->GetLight(lx, levelPos.y, lz, r, g, b);
}
