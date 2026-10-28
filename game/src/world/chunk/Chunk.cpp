#include "Chunk.h"

#include <algorithm>

#include "../biome/BiomeRegistry.h"
#include "../block/BlockRegistry.h"

Chunk::Chunk(const ChunkPos &pos)
{
    m_pos          = pos;
    m_needsRelight = true;
    m_dirty        = false;
    m_saved        = false;

    m_liquidTicksCollected = false;
    m_pendingMobSpawn      = false;

    uint8_t plains = (uint8_t) BiomeRegistry::GetInstance()->IdOf(Biome::ByName("plains"));
    for (int section = 0; section < s_sectionCount; section++)
    {
        m_sections[section].FillBiomeId(plains);
    }
}

void Chunk::MarkDirty()
{
    m_dirty = true;
}

void Chunk::ClearDirty()
{
    m_dirty = false;
}

bool Chunk::IsDirty() const
{
    return m_dirty;
}

void Chunk::SetSaved(bool saved)
{
    m_saved = saved;
}

bool Chunk::IsSaved() const
{
    return m_saved;
}

void Chunk::SetPendingMobSpawn(bool pendingMobSpawn)
{
    m_pendingMobSpawn = pendingMobSpawn;
}

bool Chunk::IsPendingMobSpawn() const
{
    return m_pendingMobSpawn;
}

bool Chunk::NeedsSave() const
{
    return m_dirty || !m_saved;
}

uint32_t Chunk::GetBlockId(int x, int y, int z) const
{
    if (y < 0 || y >= s_sizeY)
    {
        return 0;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    return m_sections[section].GetBlockId(x, localY, z);
}

void Chunk::SetBlock(int x, int y, int z, Block *block)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    uint32_t id = BlockRegistry::GetInstance()->IdOf(block);
    m_sections[section].SetBlockId(x, localY, z, id);
    m_dirty = true;
}

void Chunk::SetBlockId(int x, int y, int z, uint32_t id)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    m_sections[section].SetBlockId(x, localY, z, id);
    m_dirty = true;
}

uint16_t Chunk::GetBlockData(int x, int y, int z) const
{
    if (y < 0 || y >= s_sizeY)
    {
        return 0;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    return m_sections[section].GetBlockData(x, localY, z);
}

void Chunk::SetBlockData(int x, int y, int z, uint16_t data)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    m_sections[section].SetBlockData(x, localY, z, data);
    m_dirty = true;
}

const ChunkPos &Chunk::GetPos() const
{
    return m_pos;
}

void Chunk::SetPos(const ChunkPos &pos)
{
    m_pos = pos;
}

uint32_t Chunk::CalculateNetworkHash() const
{
    const uint32_t fnvPrime = 16777619u;
    uint32_t hash           = 2166136261u;

    for (int z = 0; z < s_sizeZ; z++)
    {
        for (int y = 0; y < s_sizeY; y++)
        {
            for (int x = 0; x < s_sizeX; x++)
            {
                uint32_t block = GetBlockId(x, y, z);
                hash ^= block & 0xFFu;
                hash *= fnvPrime;
                hash ^= (block >> 8) & 0xFFu;
                hash *= fnvPrime;
                hash ^= (block >> 16) & 0xFFu;
                hash *= fnvPrime;
                hash ^= (block >> 24) & 0xFFu;
                hash *= fnvPrime;
                hash ^= GetSkyLight(x, y, z);
                hash *= fnvPrime;
            }
        }
    }

    for (int section = 0; section < s_sectionCount; section++)
    {
        for (int y = 0; y < ChunkSection::s_size; y += ChunkSection::s_biomeHeight)
        {
            for (int z = 0; z < s_sizeZ; z++)
            {
                for (int x = 0; x < s_sizeX; x++)
                {
                    hash ^= m_sections[section].GetBiomeId(x, y, z);
                    hash *= fnvPrime;
                }
            }
        }
    }

    return hash;
}

int Chunk::GetDebugSurfaceHeight(int x, int z) const
{
    for (int y = s_sizeY - 1; y >= 0; y--)
    {
        if (GetBlockId(x, y, z) != 0)
        {
            return y;
        }
    }

    return -1;
}

int Chunk::GetHighestNonAirY() const
{
    for (int section = s_sectionCount - 1; section >= 0; section--)
    {
        if (m_sections[section].IsEmpty())
        {
            continue;
        }

        for (int localY = ChunkSection::s_size - 1; localY >= 0; localY--)
        {
            for (int z = 0; z < s_sizeZ; z++)
            {
                for (int x = 0; x < s_sizeX; x++)
                {
                    if (m_sections[section].GetBlockId(x, localY, z) != 0)
                    {
                        return section * ChunkSection::s_size + localY;
                    }
                }
            }
        }
    }

    return -1;
}

bool Chunk::IsSectionEmpty(int section) const
{
    return m_sections[section].IsEmpty();
}

void Chunk::SetBiomeAt(int x, int y, int z, Biome *biome)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }

    uint8_t id = (uint8_t) BiomeRegistry::GetInstance()->IdOf(biome);
    m_sections[y / ChunkSection::s_size].SetBiomeId(x, y % ChunkSection::s_size, z, id);
}

Biome *Chunk::GetBiomeAt(int x, int y, int z) const
{
    y          = std::clamp(y, 0, s_sizeY - 1);
    uint8_t id = m_sections[y / ChunkSection::s_size].GetBiomeId(x, y % ChunkSection::s_size, z);
    return BiomeRegistry::GetInstance()->ById(id);
}

void Chunk::SetColumnBiome(int x, int z, Biome *biome)
{
    uint8_t id = (uint8_t) BiomeRegistry::GetInstance()->IdOf(biome);
    for (int section = 0; section < s_sectionCount; section++)
    {
        for (int y = 0; y < ChunkSection::s_size; y += ChunkSection::s_biomeHeight)
        {
            m_sections[section].SetBiomeId(x, y, z, id);
        }
    }
}

Biome *Chunk::GetSurfaceBiome(int x, int z) const
{
    return GetBiomeAt(x, s_sizeY - 1, z);
}

// Each section is a 0 byte and one id when uniform, or a 1 byte and every cell id
std::vector<uint8_t> Chunk::EncodeBiomes() const
{
    std::vector<uint8_t> data;
    for (int section = 0; section < s_sectionCount; section++)
    {
        const ChunkSection &chunkSection = m_sections[section];
        if (chunkSection.IsBiomeUniform())
        {
            data.push_back(0);
            data.push_back(chunkSection.GetBiomeId(0, 0, 0));
            continue;
        }

        data.push_back(1);
        for (int y = 0; y < ChunkSection::s_size; y += ChunkSection::s_biomeHeight)
        {
            for (int z = 0; z < s_sizeZ; z++)
            {
                for (int x = 0; x < s_sizeX; x++)
                {
                    data.push_back(chunkSection.GetBiomeId(x, y, z));
                }
            }
        }
    }

    return data;
}

bool Chunk::DecodeBiomes(const std::vector<uint8_t> &data)
{
    BiomeRegistry *registry = BiomeRegistry::GetInstance();
    size_t offset           = 0;
    for (int section = 0; section < s_sectionCount; section++)
    {
        if (offset + 2 > data.size())
        {
            return false;
        }

        ChunkSection &chunkSection = m_sections[section];
        if (data[offset] == 0)
        {
            if (!registry->ById(data[offset + 1]))
            {
                return false;
            }

            chunkSection.FillBiomeId(data[offset + 1]);
            offset += 2;
            continue;
        }

        offset++;
        if (offset + ChunkSection::s_biomeCells > data.size())
        {
            return false;
        }

        for (int y = 0; y < ChunkSection::s_size; y += ChunkSection::s_biomeHeight)
        {
            for (int z = 0; z < s_sizeZ; z++)
            {
                for (int x = 0; x < s_sizeX; x++)
                {
                    if (!registry->ById(data[offset]))
                    {
                        return false;
                    }

                    chunkSection.SetBiomeId(x, y, z, data[offset]);
                    offset++;
                }
            }
        }
    }

    return offset == data.size();
}

void Chunk::GetLight(int x, int y, int z, uint8_t *r, uint8_t *g, uint8_t *b) const
{
    uint8_t br;
    uint8_t bg;
    uint8_t bb;
    GetBlockLight(x, y, z, &br, &bg, &bb);

    uint8_t sky = GetSkyLight(x, y, z);
    *r          = br > sky ? br : sky;
    *g          = bg > sky ? bg : sky;
    *b          = bb > sky ? bb : sky;
}

void Chunk::SetLight(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    m_sections[section].SetBlockLight(x, localY, z, r, g, b);
}

void Chunk::GetBlockLight(int x, int y, int z, uint8_t *r, uint8_t *g, uint8_t *b) const
{
    if (y < 0 || y >= s_sizeY)
    {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    m_sections[section].GetBlockLight(x, localY, z, r, g, b);
}

void Chunk::SetBlockLight(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    m_sections[section].SetBlockLight(x, localY, z, r, g, b);
}

uint8_t Chunk::GetSkyLight(int x, int y, int z) const
{
    if (y < 0 || y >= s_sizeY)
    {
        return y < 0 ? 0 : 15;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    return m_sections[section].GetSkyLight(x, localY, z);
}

void Chunk::SetSkyLight(int x, int y, int z, uint8_t level)
{
    if (y < 0 || y >= s_sizeY)
    {
        return;
    }
    int section = y / ChunkSection::s_size;
    int localY  = y % ChunkSection::s_size;
    m_sections[section].SetSkyLight(x, localY, z, level);
}

void Chunk::CollectLiquidTicks()
{
    m_liquidTicks.clear();
    m_liquidTicksCollected = true;

    uint32_t waterFlowId = BlockRegistry::GetInstance()->IdOf(Block::ByName("water_flow"));
    uint32_t lavaFlowId  = BlockRegistry::GetInstance()->IdOf(Block::ByName("lava_flow"));

    for (int section = 0; section < s_sectionCount; section++)
    {
        if (m_sections[section].IsEmpty())
        {
            continue;
        }

        for (int localY = 0; localY < ChunkSection::s_size; localY++)
        {
            int y = section * ChunkSection::s_size + localY;
            if (y < 1)
            {
                continue;
            }

            for (int z = 0; z < s_sizeZ; z++)
            {
                for (int x = 0; x < s_sizeX; x++)
                {
                    uint32_t blockId = m_sections[section].GetBlockId(x, localY, z);
                    if (blockId == waterFlowId || blockId == lavaFlowId)
                    {
                        m_liquidTicks.push_back((uint16_t) ((y << 8) | (z << 4) | x));
                    }
                }
            }
        }
    }
}

const std::vector<uint16_t> &Chunk::TakeLiquidTicks()
{
    if (!m_liquidTicksCollected)
    {
        CollectLiquidTicks();
    }

    return m_liquidTicks;
}

void Chunk::ClearLiquidTicks()
{
    m_liquidTicks.clear();
    m_liquidTicks.shrink_to_fit();
    m_liquidTicksCollected = true;
}

void Chunk::SetBlockTicks(std::vector<ChunkBlockTick> blockTicks)
{
    m_blockTicks = std::move(blockTicks);
}

std::vector<ChunkBlockTick> Chunk::TakeBlockTicks()
{
    std::vector<ChunkBlockTick> blockTicks = std::move(m_blockTicks);
    m_blockTicks.clear();
    return blockTicks;
}

const std::vector<ChunkBlockTick> &Chunk::GetBlockTicks() const
{
    return m_blockTicks;
}

void Chunk::SetBlockEntities(std::vector<std::shared_ptr<CompoundTag>> blockEntities)
{
    m_blockEntities = std::move(blockEntities);
}

std::vector<std::shared_ptr<CompoundTag>> Chunk::TakeBlockEntities()
{
    std::vector<std::shared_ptr<CompoundTag>> blockEntities = std::move(m_blockEntities);
    m_blockEntities.clear();
    return blockEntities;
}

const std::vector<std::shared_ptr<CompoundTag>> &Chunk::GetBlockEntities() const
{
    return m_blockEntities;
}

const ChunkSection &Chunk::GetSection(int section) const
{
    return m_sections[section];
}

ChunkSection &Chunk::GetSection(int section)
{
    return m_sections[section];
}

void Chunk::Compact()
{
    for (int section = 0; section < s_sectionCount; section++)
    {
        m_sections[section].Compact();
    }
}

size_t Chunk::GetAllocatedBytes() const
{
    size_t bytes = 0;
    for (int section = 0; section < s_sectionCount; section++)
    {
        bytes += m_sections[section].GetAllocatedBytes();
    }

    return bytes;
}
