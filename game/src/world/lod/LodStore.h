#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../../io/File.h"
#include "LodVolume.h"

class Chunk;

class LodStore
{
public:
    LodStore();

    void Open(const std::string &worldName, bool inMemory);
    std::string GetWorldName();
    void Ingest(const std::string &worldName, const Chunk *chunk, bool replace);
    bool ReadChunk(int chunkX, int chunkZ, LodVolume *volume);
    bool TryProbe(int chunkX, int chunkZ);
    bool HasChunks(int minChunkX, int minChunkZ, int maxChunkX, int maxChunkZ);
    bool ReadTile(int level, int tileX, int tileZ, LodVolume *volume);
    void WriteTile(int level, int tileX, int tileZ, const LodVolume &volume);

private:
    struct Group
    {
        std::vector<std::vector<uint8_t>> blobs;
        std::vector<bool> probed;
        int present;
        bool worldRegion;
    };

    static const int s_regionChunks = 32;
    static const int s_regionSlots  = s_regionChunks * s_regionChunks;
    static const int s_tileGrid     = 16;
    static const int s_tileSlots    = s_tileGrid * s_tileGrid;
    static const size_t s_maxGroups = 128;

    static uint64_t MakeKey(int x, int z);

    Group *GetRegion(int regionX, int regionZ);
    Group *GetTileGroup(int level, int gridX, int gridZ);
    File GetRegionFile(int regionX, int regionZ) const;
    File GetTileFile(int level, int gridX, int gridZ) const;
    void LoadGroup(const File &file, int slotCount, Group *group);
    void WriteBlob(const File &file, int slotCount, int slot, const std::vector<uint8_t> &bytes);

    std::mutex m_mutex;
    std::string m_worldName;
    File m_directory;
    File m_worldRegionDirectory;
    bool m_inMemory;
    std::unordered_map<uint64_t, Group> m_regions;
    std::map<std::tuple<int, int, int>, Group> m_tileGroups;
};
