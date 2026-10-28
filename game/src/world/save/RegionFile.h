#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class RegionFile
{
public:
    static const int s_regionSize = 32;

    static int LocalIndex(int chunkX, int chunkZ);
    static bool ReadChunk(const std::string &path, int localIndex, std::vector<uint8_t> *outBlob);
    static void WriteChunk(const std::string &path, int localIndex, const std::vector<uint8_t> &blob);
    static void WriteChunks(const std::string &path, const std::vector<std::pair<int, std::vector<uint8_t>>> &blobs);

private:
    static const int s_chunkCount = s_regionSize * s_regionSize;
    static const int s_entrySize  = 12;
    static const int s_headerSize = s_chunkCount * s_entrySize;
};
