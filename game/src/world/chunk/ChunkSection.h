#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../../utils/Random.h"

class ChunkSection
{
public:
    static const int s_size   = 16;
    static const int s_volume = s_size * s_size * s_size;

    static const int s_biomeHeight = 4;
    static const int s_biomeCells  = s_size * s_size * (s_size / s_biomeHeight);

    ChunkSection();
    ~ChunkSection();
    ChunkSection(const ChunkSection &other);
    ChunkSection &operator=(const ChunkSection &other);

    static int Index(int x, int y, int z);
    static uint32_t MakeState(uint32_t id, uint16_t data);
    static uint32_t StateId(uint32_t state);
    static uint16_t StateData(uint32_t state);

    uint32_t GetState(int index) const;
    void SetState(int index, uint32_t state);

    uint32_t GetBlockId(int x, int y, int z) const;
    void SetBlockId(int x, int y, int z, uint32_t id);

    uint16_t GetBlockData(int x, int y, int z) const;
    void SetBlockData(int x, int y, int z, uint16_t data);

    uint8_t GetSkyLight(int x, int y, int z) const;
    void SetSkyLight(int x, int y, int z, uint8_t level);
    void FillSkyLight(uint8_t level);

    void GetBlockLight(int x, int y, int z, uint8_t *r, uint8_t *g, uint8_t *b) const;
    void SetBlockLight(int x, int y, int z, uint8_t r, uint8_t g, uint8_t b);

    uint8_t GetBiomeId(int x, int y, int z) const;
    void SetBiomeId(int x, int y, int z, uint8_t id);
    void FillBiomeId(uint8_t id);
    bool IsBiomeUniform() const;

    bool IsEmpty() const;
    bool ContainsBlock(uint32_t id) const;
    void PickRandomTicks(Random *random, int count, std::vector<uint16_t> *out) const;

    void Compact();
    size_t GetAllocatedBytes() const;

private:
    static constexpr int kMinIndexBits = 4;
    static constexpr int kMaxIndexBits = 16;

    struct BlockStorage
    {
        BlockStorage *previous;
        uint32_t *palette;
        uint8_t *indices;
        int bits;
        int paletteSize;
        int paletteCapacity;
    };

    struct LightData
    {
        uint8_t redGreen[s_volume];
        uint8_t blue[s_volume / 2];
    };

    static BlockStorage *CreateBlockStorage(int bits);
    static void DestroyBlockStorage(BlockStorage *storage);
    static size_t GetBlockStorageBytes(int bits);
    static int GetPaletteCapacity(int bits);
    static int ReadIndex(const BlockStorage *storage, int index);
    static void WriteIndex(BlockStorage *storage, int index, int paletteIndex);
    static int BiomeIndex(int x, int y, int z);
    static uint8_t ReadNibble(const uint8_t *nibbles, int index);
    static void WriteNibble(uint8_t *nibbles, int index, uint8_t value);

    int FindOrAddPaletteIndex(uint32_t state);
    void RemovePaletteGarbage();
    void Repack(int bits, const uint32_t *palette, int paletteSize, const int *remap);
    LightData *EnsureBlockLightStorage();
    uint8_t *EnsureSkyLightStorage();

    void ReleaseStorage();
    void CopyFrom(const ChunkSection &other);

    std::atomic<BlockStorage *> m_blockStorage;
    std::atomic<LightData *> m_blockLightStorage;
    std::atomic<uint8_t *> m_skyLightStorage;
    std::atomic<uint8_t> m_uniformSkyLight;
    std::atomic<uint8_t *> m_biomeStorage;
    std::atomic<uint8_t> m_uniformBiome;
    int m_nonAirCount;
};
