#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../biome/Biome.h"
#include "../block/Block.h"
#include "ChunkPos.h"
#include "ChunkSection.h"

class CompoundTag;

struct ChunkBlockTick
{
    Block *block;
    int x;
    int y;
    int z;
    int delay;
    int priority;
};

class Chunk
{
public:
    static const int s_sizeX        = 16;
    static inline const int s_sizeY = 256;
    static const int s_sizeZ        = 16;
    static const int s_sectionCount = s_sizeY / ChunkSection::s_size;

    explicit Chunk(const ChunkPos &pos);

    uint32_t GetBlockId(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, Block *block);
    void SetBlockId(int x, int y, int z, uint32_t id);
    uint16_t GetBlockData(int x, int y, int z) const;
    void SetBlockData(int x, int y, int z, uint16_t data);

    const ChunkPos &GetPos() const;
    void SetPos(const ChunkPos &pos);

    void MarkDirty();
    void ClearDirty();
    bool IsDirty() const;
    void SetSaved(bool saved);
    bool IsSaved() const;
    bool NeedsSave() const;
    void SetPendingMobSpawn(bool pendingMobSpawn);
    bool IsPendingMobSpawn() const;

    uint32_t CalculateNetworkHash() const;
    int GetDebugSurfaceHeight(int x, int z) const;
    int GetHighestNonAirY() const;
    bool IsSectionEmpty(int section) const;
    const ChunkSection &GetSection(int section) const;
    ChunkSection &GetSection(int section);

    void SetBiomeAt(int x, int y, int z, Biome *biome);
    Biome *GetBiomeAt(int x, int y, int z) const;
    void SetColumnBiome(int x, int z, Biome *biome);
    Biome *GetSurfaceBiome(int x, int z) const;
    std::vector<uint8_t> EncodeBiomes() const;
    bool DecodeBiomes(const std::vector<uint8_t> &data);

    void GetLight(int x, int y, int z, uint8_t *r, uint8_t *g, uint8_t *b) const;
    void SetLight(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b);

    void GetBlockLight(int x, int y, int z, uint8_t *r, uint8_t *g, uint8_t *b) const;
    void SetBlockLight(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b);

    uint8_t GetSkyLight(int x, int y, int z) const;
    void SetSkyLight(int x, int y, int z, uint8_t level);

    void Compact();
    size_t GetAllocatedBytes() const;

    void CollectLiquidTicks();
    const std::vector<uint16_t> &TakeLiquidTicks();
    void ClearLiquidTicks();

    void SetBlockTicks(std::vector<ChunkBlockTick> blockTicks);
    std::vector<ChunkBlockTick> TakeBlockTicks();
    const std::vector<ChunkBlockTick> &GetBlockTicks() const;

    void SetBlockEntities(std::vector<std::shared_ptr<CompoundTag>> blockEntities);
    std::vector<std::shared_ptr<CompoundTag>> TakeBlockEntities();
    const std::vector<std::shared_ptr<CompoundTag>> &GetBlockEntities() const;

private:
    ChunkPos m_pos;
    bool m_needsRelight;
    bool m_dirty;
    bool m_saved;
    bool m_liquidTicksCollected;
    bool m_pendingMobSpawn;

    std::vector<uint16_t> m_liquidTicks;
    std::vector<ChunkBlockTick> m_blockTicks;
    std::vector<std::shared_ptr<CompoundTag>> m_blockEntities;

    ChunkSection m_sections[s_sectionCount];
};
