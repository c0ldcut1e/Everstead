#pragma once

#include <cstdint>
#include <vector>

class LodVolume
{
public:
    static const uint8_t s_columnGenerated = 1;
    static const uint8_t s_columnMissing   = 2;

    LodVolume();
    LodVolume(int sizeX, int sizeY, int sizeZ);

    int GetSizeX() const;
    int GetSizeY() const;
    int GetSizeZ() const;
    uint16_t GetBlock(int x, int y, int z) const;
    void SetBlock(int x, int y, int z, uint16_t blockId);
    uint8_t GetBiome(int x, int z) const;
    void SetBiome(int x, int z, uint8_t biome);
    uint8_t GetFlags(int x, int z) const;
    void SetFlags(int x, int z, uint8_t flags);
    bool HasColumnFlag(uint8_t flag) const;

    std::vector<uint8_t> Encode() const;
    bool Decode(const std::vector<uint8_t> &bytes);

    static uint16_t PickBlock(const uint16_t *blocks, int count);

private:
    int m_sizeX;
    int m_sizeY;
    int m_sizeZ;
    std::vector<uint16_t> m_blocks;
    std::vector<uint8_t> m_biomes;
    std::vector<uint8_t> m_flags;
};
