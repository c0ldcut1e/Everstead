#include "LodVolume.h"

#include <unordered_map>

#include "../block/BlockRegistry.h"

#include <zlib.h>

LodVolume::LodVolume()
{
    m_sizeX = 0;
    m_sizeY = 0;
    m_sizeZ = 0;
}

LodVolume::LodVolume(int sizeX, int sizeY, int sizeZ)
{
    m_sizeX = sizeX;
    m_sizeY = sizeY;
    m_sizeZ = sizeZ;
    m_blocks.assign((size_t) sizeX * sizeY * sizeZ, 0);
    m_biomes.assign((size_t) sizeX * sizeZ, 0);
    m_flags.assign((size_t) sizeX * sizeZ, 0);
}

int LodVolume::GetSizeX() const
{
    return m_sizeX;
}

int LodVolume::GetSizeY() const
{
    return m_sizeY;
}

int LodVolume::GetSizeZ() const
{
    return m_sizeZ;
}

uint16_t LodVolume::GetBlock(int x, int y, int z) const
{
    return m_blocks[((size_t) y * m_sizeZ + z) * m_sizeX + x];
}

void LodVolume::SetBlock(int x, int y, int z, uint16_t blockId)
{
    m_blocks[((size_t) y * m_sizeZ + z) * m_sizeX + x] = blockId;
}

uint8_t LodVolume::GetBiome(int x, int z) const
{
    return m_biomes[(size_t) z * m_sizeX + x];
}

void LodVolume::SetBiome(int x, int z, uint8_t biome)
{
    m_biomes[(size_t) z * m_sizeX + x] = biome;
}

uint8_t LodVolume::GetFlags(int x, int z) const
{
    return m_flags[(size_t) z * m_sizeX + x];
}

void LodVolume::SetFlags(int x, int z, uint8_t flags)
{
    m_flags[(size_t) z * m_sizeX + x] = flags;
}

bool LodVolume::HasColumnFlag(uint8_t flag) const
{
    for (size_t i = 0; i < m_flags.size(); i++)
    {
        if (m_flags[i] & flag)
        {
            return true;
        }
    }

    return false;
}

std::vector<uint8_t> LodVolume::Encode() const
{
    std::vector<uint16_t> palette;
    std::unordered_map<uint16_t, uint16_t> lookup;
    std::vector<uint16_t> indices(m_blocks.size());
    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        auto found = lookup.find(m_blocks[i]);
        if (found == lookup.end())
        {
            found = lookup.emplace(m_blocks[i], (uint16_t) palette.size()).first;
            palette.push_back(m_blocks[i]);
        }

        indices[i] = found->second;
    }

    bool wide = palette.size() > 256;
    std::vector<uint8_t> raw;
    raw.reserve(2 + palette.size() * 2 + m_biomes.size() + m_flags.size() + indices.size() * (wide ? 2 : 1));
    raw.push_back((uint8_t) palette.size());
    raw.push_back((uint8_t) (palette.size() >> 8));
    for (size_t i = 0; i < palette.size(); i++)
    {
        raw.push_back((uint8_t) palette[i]);
        raw.push_back((uint8_t) (palette[i] >> 8));
    }

    raw.insert(raw.end(), m_biomes.begin(), m_biomes.end());
    raw.insert(raw.end(), m_flags.begin(), m_flags.end());
    for (size_t i = 0; i < indices.size(); i++)
    {
        raw.push_back((uint8_t) indices[i]);
        if (wide)
        {
            raw.push_back((uint8_t) (indices[i] >> 8));
        }
    }

    uLongf compressedSize = compressBound((uLong) raw.size());
    std::vector<uint8_t> bytes(8 + (size_t) compressedSize);
    bytes[0] = (uint8_t) m_sizeX;
    bytes[1] = (uint8_t) m_sizeY;
    bytes[2] = (uint8_t) (m_sizeY >> 8);
    bytes[3] = (uint8_t) m_sizeZ;
    bytes[4] = (uint8_t) raw.size();
    bytes[5] = (uint8_t) (raw.size() >> 8);
    bytes[6] = (uint8_t) (raw.size() >> 16);
    bytes[7] = (uint8_t) (raw.size() >> 24);
    compress2(bytes.data() + 8, &compressedSize, raw.data(), (uLong) raw.size(), Z_BEST_SPEED);
    bytes.resize(8 + (size_t) compressedSize);
    return bytes;
}

bool LodVolume::Decode(const std::vector<uint8_t> &bytes)
{
    if (bytes.size() < 8)
    {
        return false;
    }

    int sizeX      = bytes[0];
    int sizeY      = bytes[1] | bytes[2] << 8;
    int sizeZ      = bytes[3];
    size_t rawSize = (size_t) bytes[4] | (size_t) bytes[5] << 8 | (size_t) bytes[6] << 16 | (size_t) bytes[7] << 24;
    size_t columns = (size_t) sizeX * sizeZ;
    size_t voxels  = columns * sizeY;
    if (sizeX == 0 || sizeZ == 0 || sizeY == 0 || sizeY > 256 || rawSize > 2 + 65536 * 2 + columns * 2 + voxels * 2)
    {
        return false;
    }

    std::vector<uint8_t> raw(rawSize);
    uLongf rawLength = (uLongf) rawSize;
    if (rawSize < 2 || uncompress(raw.data(), &rawLength, bytes.data() + 8, (uLong) (bytes.size() - 8)) != Z_OK || rawLength != rawSize)
    {
        return false;
    }

    size_t paletteCount = raw[0] | raw[1] << 8;
    bool wide           = paletteCount > 256;
    if (paletteCount == 0 || rawSize != 2 + paletteCount * 2 + columns * 2 + voxels * (wide ? 2 : 1))
    {
        return false;
    }

    // Cached tiles come from disk, so every id is checked against the registry before the mesher looks it up
    uint32_t blockCount = BlockRegistry::GetInstance()->Size();
    std::vector<uint16_t> palette(paletteCount);
    for (size_t i = 0; i < paletteCount; i++)
    {
        palette[i] = (uint16_t) (raw[2 + i * 2] | raw[3 + i * 2] << 8);
        if (palette[i] >= blockCount)
        {
            return false;
        }
    }

    size_t offset = 2 + paletteCount * 2;
    m_sizeX       = sizeX;
    m_sizeY       = sizeY;
    m_sizeZ       = sizeZ;
    m_biomes.assign(raw.begin() + (std::ptrdiff_t) offset, raw.begin() + (std::ptrdiff_t) (offset + columns));
    m_flags.assign(raw.begin() + (std::ptrdiff_t) (offset + columns), raw.begin() + (std::ptrdiff_t) (offset + columns * 2));
    offset += columns * 2;
    m_blocks.resize(voxels);
    for (size_t i = 0; i < voxels; i++)
    {
        size_t index = wide ? (size_t) (raw[offset + i * 2] | raw[offset + i * 2 + 1] << 8) : raw[offset + i];
        if (index >= paletteCount)
        {
            return false;
        }

        m_blocks[i] = palette[index];
    }

    return true;
}

uint16_t LodVolume::PickBlock(const uint16_t *blocks, int count)
{
    uint16_t best = 0;
    int bestVotes = 0;
    for (int i = 0; i < count; i++)
    {
        if (blocks[i] == 0)
        {
            continue;
        }

        int votes = 0;
        for (int j = 0; j < count; j++)
        {
            votes += blocks[j] == blocks[i];
        }

        if (votes > bestVotes)
        {
            best      = blocks[i];
            bestVotes = votes;
        }
    }

    return best;
}
