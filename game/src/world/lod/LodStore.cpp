#include "LodStore.h"

#include <algorithm>
#include <fstream>

#include "../../utils/math/Mth.h"
#include "../biome/BiomeRegistry.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"
#include "../save/WorldSave.h"
#include "LodGenerator.h"

LodStore::LodStore()
{
    m_inMemory = false;
}

void LodStore::Open(const std::string &worldName, bool inMemory)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_regions.clear();
    m_tileGroups.clear();
    m_worldName            = worldName;
    m_inMemory             = inMemory;
    m_directory            = worldName.empty() ? File() : WorldSave::GetWorldDirectory(worldName).GetChild("lod");
    m_worldRegionDirectory = worldName.empty() ? File() : WorldSave::GetDimensionDirectory(worldName, eDimensionId_Overworld).GetChild("region");
}

std::string LodStore::GetWorldName()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_worldName;
}

void LodStore::Ingest(const std::string &worldName, const Chunk *chunk, bool replace)
{
    if (worldName != GetWorldName())
    {
        return;
    }

    const ChunkPos &pos = chunk->GetPos();
    int regionX         = Mth::FloorDiv(pos.x, s_regionChunks);
    int regionZ         = Mth::FloorDiv(pos.z, s_regionChunks);
    int slot            = (pos.z - regionZ * s_regionChunks) * s_regionChunks + pos.x - regionX * s_regionChunks;
    if (!replace)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        Group *region = GetRegion(regionX, regionZ);
        if (!region || !region->blobs[slot].empty())
        {
            return;
        }
    }

    BiomeRegistry *biomes = BiomeRegistry::GetInstance();
    LodVolume volume(Chunk::s_sizeX, Chunk::s_sizeY, Chunk::s_sizeZ);
    int highestY = chunk->GetHighestNonAirY();
    for (int z = 0; z < Chunk::s_sizeZ; z++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            volume.SetBiome(x, z, (uint8_t) biomes->IdOf(chunk->GetSurfaceBiome(x, z)));
            for (int y = 0; y <= highestY; y++)
            {
                Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                if (block->IsSolid() || block->IsLiquid())
                {
                    volume.SetBlock(x, y, z, (uint16_t) block->GetId());
                }
            }
        }
    }

    std::vector<uint8_t> bytes = volume.Encode();
    std::lock_guard<std::mutex> lock(m_mutex);
    Group *region = GetRegion(regionX, regionZ);
    if (!region)
    {
        return;
    }

    if (region->blobs[slot].empty())
    {
        region->present++;
    }

    region->blobs[slot] = bytes;
    if (!m_inMemory)
    {
        WriteBlob(GetRegionFile(regionX, regionZ), s_regionSlots, slot, bytes);
    }

    for (int level = 0; level < LodGenerator::s_levelCount; level++)
    {
        int tileSize = LodGenerator::GetTileSize(level);
        int tileX    = Mth::FloorDiv(pos.x * Chunk::s_sizeX, tileSize);
        int tileZ    = Mth::FloorDiv(pos.z * Chunk::s_sizeZ, tileSize);
        int gridX    = Mth::FloorDiv(tileX, s_tileGrid);
        int gridZ    = Mth::FloorDiv(tileZ, s_tileGrid);
        int tileSlot = (tileZ - gridZ * s_tileGrid) * s_tileGrid + tileX - gridX * s_tileGrid;
        auto found   = m_tileGroups.find(std::make_tuple(level, gridX, gridZ));
        if (found != m_tileGroups.end())
        {
            found->second.blobs[tileSlot].clear();
        }

        File file = GetTileFile(level, gridX, gridZ);
        if (!m_inMemory && !m_directory.GetPath().empty() && file.Exists())
        {
            const char empty[8] = {};
            std::fstream stream(file.GetPath(), std::ios::binary | std::ios::in | std::ios::out);
            stream.seekp((std::streamoff) tileSlot * 8);
            stream.write(empty, 8);
        }
    }
}

bool LodStore::ReadChunk(int chunkX, int chunkZ, LodVolume *volume)
{
    int regionX = Mth::FloorDiv(chunkX, s_regionChunks);
    int regionZ = Mth::FloorDiv(chunkZ, s_regionChunks);
    int slot    = (chunkZ - regionZ * s_regionChunks) * s_regionChunks + chunkX - regionX * s_regionChunks;
    std::vector<uint8_t> bytes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        Group *region = GetRegion(regionX, regionZ);
        if (!region || region->blobs[slot].empty())
        {
            return false;
        }

        bytes = region->blobs[slot];
    }

    return volume->Decode(bytes) && volume->GetSizeX() == Chunk::s_sizeX && volume->GetSizeY() == Chunk::s_sizeY &&
           volume->GetSizeZ() == Chunk::s_sizeZ;
}

bool LodStore::TryProbe(int chunkX, int chunkZ)
{
    int regionX = Mth::FloorDiv(chunkX, s_regionChunks);
    int regionZ = Mth::FloorDiv(chunkZ, s_regionChunks);
    int slot    = (chunkZ - regionZ * s_regionChunks) * s_regionChunks + chunkX - regionX * s_regionChunks;
    std::lock_guard<std::mutex> lock(m_mutex);
    Group *region = GetRegion(regionX, regionZ);
    if (!region || !region->worldRegion || region->probed[slot])
    {
        return false;
    }

    region->probed[slot] = true;
    return true;
}

bool LodStore::HasChunks(int minChunkX, int minChunkZ, int maxChunkX, int maxChunkZ)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (int regionZ = Mth::FloorDiv(minChunkZ, s_regionChunks); regionZ <= Mth::FloorDiv(maxChunkZ, s_regionChunks); regionZ++)
    {
        for (int regionX = Mth::FloorDiv(minChunkX, s_regionChunks); regionX <= Mth::FloorDiv(maxChunkX, s_regionChunks); regionX++)
        {
            Group *region = GetRegion(regionX, regionZ);
            if (!region || (region->present == 0 && !region->worldRegion))
            {
                continue;
            }

            int startX = std::max(minChunkX - regionX * s_regionChunks, 0);
            int startZ = std::max(minChunkZ - regionZ * s_regionChunks, 0);
            int endX   = std::min(maxChunkX - regionX * s_regionChunks, s_regionChunks - 1);
            int endZ   = std::min(maxChunkZ - regionZ * s_regionChunks, s_regionChunks - 1);
            for (int z = startZ; z <= endZ; z++)
            {
                for (int x = startX; x <= endX; x++)
                {
                    int slot = z * s_regionChunks + x;
                    if (!region->blobs[slot].empty() || (region->worldRegion && !region->probed[slot]))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool LodStore::ReadTile(int level, int tileX, int tileZ, LodVolume *volume)
{
    int gridX = Mth::FloorDiv(tileX, s_tileGrid);
    int gridZ = Mth::FloorDiv(tileZ, s_tileGrid);
    int slot  = (tileZ - gridZ * s_tileGrid) * s_tileGrid + tileX - gridX * s_tileGrid;
    std::vector<uint8_t> bytes;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        Group *group = GetTileGroup(level, gridX, gridZ);
        if (!group || group->blobs[slot].empty())
        {
            return false;
        }

        bytes = group->blobs[slot];
    }

    return volume->Decode(bytes);
}

void LodStore::WriteTile(int level, int tileX, int tileZ, const LodVolume &volume)
{
    int gridX                  = Mth::FloorDiv(tileX, s_tileGrid);
    int gridZ                  = Mth::FloorDiv(tileZ, s_tileGrid);
    int slot                   = (tileZ - gridZ * s_tileGrid) * s_tileGrid + tileX - gridX * s_tileGrid;
    std::vector<uint8_t> bytes = volume.Encode();
    std::lock_guard<std::mutex> lock(m_mutex);
    Group *group = GetTileGroup(level, gridX, gridZ);
    if (!group)
    {
        return;
    }

    group->blobs[slot] = bytes;
    if (!m_inMemory)
    {
        WriteBlob(GetTileFile(level, gridX, gridZ), s_tileSlots, slot, bytes);
    }
}

uint64_t LodStore::MakeKey(int x, int z)
{
    return (uint64_t) (uint32_t) x << 32 | (uint32_t) z;
}

LodStore::Group *LodStore::GetRegion(int regionX, int regionZ)
{
    if (!m_inMemory && m_directory.GetPath().empty())
    {
        return nullptr;
    }

    uint64_t key = MakeKey(regionX, regionZ);
    auto found   = m_regions.find(key);
    if (found != m_regions.end())
    {
        return &found->second;
    }

    // Far tiles touch many regions, so we drop the whole cache once it grows instead of tracking use order
    if (!m_inMemory && m_regions.size() >= s_maxGroups)
    {
        m_regions.clear();
    }

    Group *region = &m_regions[key];
    region->blobs.resize(s_regionSlots);
    region->probed.assign(s_regionSlots, false);
    region->present     = 0;
    region->worldRegion = false;
    if (!m_inMemory)
    {
        LoadGroup(GetRegionFile(regionX, regionZ), s_regionSlots, region);
        region->worldRegion = m_worldRegionDirectory.GetChild("r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".evr").Exists();
    }

    return region;
}

LodStore::Group *LodStore::GetTileGroup(int level, int gridX, int gridZ)
{
    if (!m_inMemory && m_directory.GetPath().empty())
    {
        return nullptr;
    }

    std::tuple<int, int, int> key = std::make_tuple(level, gridX, gridZ);
    auto found                    = m_tileGroups.find(key);
    if (found != m_tileGroups.end())
    {
        return &found->second;
    }

    if (m_tileGroups.size() >= s_maxGroups)
    {
        m_tileGroups.clear();
    }

    Group *group = &m_tileGroups[key];
    group->blobs.resize(s_tileSlots);
    group->present     = 0;
    group->worldRegion = false;
    if (!m_inMemory)
    {
        LoadGroup(GetTileFile(level, gridX, gridZ), s_tileSlots, group);
    }

    return group;
}

File LodStore::GetRegionFile(int regionX, int regionZ) const
{
    return m_directory.GetChild("b." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".dat");
}

File LodStore::GetTileFile(int level, int gridX, int gridZ) const
{
    return m_directory.GetChild("c." + std::to_string(level) + "." + std::to_string(gridX) + "." + std::to_string(gridZ) + ".dat");
}

void LodStore::LoadGroup(const File &file, int slotCount, Group *group)
{
    std::vector<uint8_t> bytes = file.ReadAllBytes();
    size_t headerSize          = (size_t) slotCount * 8;
    if (bytes.size() < headerSize)
    {
        return;
    }

    size_t live = 0;
    for (int slot = 0; slot < slotCount; slot++)
    {
        const uint8_t *entry = &bytes[(size_t) slot * 8];
        size_t offset        = (size_t) entry[0] | (size_t) entry[1] << 8 | (size_t) entry[2] << 16 | (size_t) entry[3] << 24;
        size_t size          = (size_t) entry[4] | (size_t) entry[5] << 8 | (size_t) entry[6] << 16 | (size_t) entry[7] << 24;
        if (size == 0 || offset < headerSize || offset + size > bytes.size())
        {
            continue;
        }

        group->blobs[slot].assign(bytes.begin() + (std::ptrdiff_t) offset, bytes.begin() + (std::ptrdiff_t) (offset + size));
        group->present++;
        live += size;
    }

    // Rewritten slots leave their old data behind in the file, so we compact once the dead space adds up
    if (bytes.size() <= headerSize + live * 2 + 65536)
    {
        return;
    }

    std::vector<uint8_t> compacted(headerSize);
    for (int slot = 0; slot < slotCount; slot++)
    {
        const std::vector<uint8_t> &blob = group->blobs[slot];
        if (blob.empty())
        {
            continue;
        }

        uint8_t *entry = &compacted[(size_t) slot * 8];
        size_t offset  = compacted.size();
        for (int i = 0; i < 4; i++)
        {
            entry[i]     = (uint8_t) (offset >> (i * 8));
            entry[i + 4] = (uint8_t) (blob.size() >> (i * 8));
        }

        compacted.insert(compacted.end(), blob.begin(), blob.end());
    }

    File temp(file.GetPath() + ".tmp");
    if (temp.WriteAllBytes(compacted))
    {
        temp.RenameTo(file);
    }
}

void LodStore::WriteBlob(const File &file, int slotCount, int slot, const std::vector<uint8_t> &bytes)
{
    if (!file.Exists())
    {
        m_directory.CreateDirectories();
        file.WriteAllBytes(std::vector<uint8_t>((size_t) slotCount * 8));
    }

    // We append the data before pointing the header at it so a crash mid write leaves the old entry intact
    std::fstream stream(file.GetPath(), std::ios::binary | std::ios::in | std::ios::out);
    stream.seekp(0, std::ios::end);
    size_t offset = (size_t) stream.tellp();
    stream.write((const char *) bytes.data(), (std::streamsize) bytes.size());
    char entry[8];
    for (int i = 0; i < 4; i++)
    {
        entry[i]     = (char) (offset >> (i * 8));
        entry[i + 4] = (char) (bytes.size() >> (i * 8));
    }

    stream.seekp((std::streamoff) slot * 8);
    stream.write(entry, 8);
}
