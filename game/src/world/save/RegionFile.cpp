#include "RegionFile.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <mutex>

#include "../../utils/math/Mth.h"

static std::mutex s_regionMutex;

int RegionFile::LocalIndex(int chunkX, int chunkZ)
{
    int localX = Mth::FloorMod(chunkX, s_regionSize);
    int localZ = Mth::FloorMod(chunkZ, s_regionSize);
    return localZ * s_regionSize + localX;
}

bool RegionFile::ReadChunk(const std::string &path, int localIndex, std::vector<uint8_t> *outBlob)
{
    std::lock_guard<std::mutex> lock(s_regionMutex);

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    file.seekg((std::streamoff) localIndex * s_entrySize);
    uint8_t entry[s_entrySize];
    if (!file.read((char *) entry, s_entrySize))
    {
        return false;
    }

    uint32_t offset;
    uint32_t length;
    memcpy(&offset, entry, 4);
    memcpy(&length, entry + 4, 4);
    if (offset == 0 || length == 0)
    {
        return false;
    }

    file.seekg((std::streamoff) offset);
    outBlob->resize(length);
    if (!file.read((char *) outBlob->data(), (std::streamsize) length))
    {
        return false;
    }

    return true;
}

void RegionFile::WriteChunk(const std::string &path, int localIndex, const std::vector<uint8_t> &blob)
{
    std::lock_guard<std::mutex> lock(s_regionMutex);

    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        std::ofstream create(path, std::ios::binary | std::ios::trunc);
        std::vector<uint8_t> header((size_t) s_headerSize, 0);
        create.write((const char *) header.data(), (std::streamsize) header.size());
        create.close();

        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            return;
        }
    }

    file.seekg((std::streamoff) localIndex * s_entrySize);
    uint8_t entry[s_entrySize];
    uint32_t offset   = 0;
    uint32_t length   = 0;
    uint32_t capacity = 0;
    if (file.read((char *) entry, s_entrySize))
    {
        memcpy(&offset, entry, 4);
        memcpy(&length, entry + 4, 4);
        memcpy(&capacity, entry + 8, 4);
    }
    file.clear();

    uint32_t newLength = (uint32_t) blob.size();
    uint32_t writeOffset;
    uint32_t newCapacity;
    if (offset != 0 && newLength <= capacity)
    {
        writeOffset = offset;
        newCapacity = capacity;
    }
    else
    {
        file.seekp(0, std::ios::end);
        writeOffset = (uint32_t) file.tellp();
        if (writeOffset < (uint32_t) s_headerSize)
        {
            writeOffset = (uint32_t) s_headerSize;
        }

        newCapacity = newLength;
    }

    file.seekp((std::streamoff) writeOffset);
    file.write((const char *) blob.data(), (std::streamsize) blob.size());

    memcpy(entry, &writeOffset, 4);
    memcpy(entry + 4, &newLength, 4);
    memcpy(entry + 8, &newCapacity, 4);
    file.seekp((std::streamoff) localIndex * s_entrySize);
    file.write((const char *) entry, s_entrySize);
    file.flush();
}

void RegionFile::WriteChunks(const std::string &path, const std::vector<std::pair<int, std::vector<uint8_t>>> &blobs)
{
    std::lock_guard<std::mutex> lock(s_regionMutex);

    std::vector<uint8_t> header((size_t) s_headerSize, 0);
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (file.is_open())
    {
        file.read((char *) header.data(), (std::streamsize) header.size());
        file.clear();
    }
    else
    {
        std::ofstream create(path, std::ios::binary | std::ios::trunc);
        create.write((const char *) header.data(), (std::streamsize) header.size());
        create.close();

        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            return;
        }
    }

    file.seekp(0, std::ios::end);
    uint32_t end = std::max((uint32_t) file.tellp(), (uint32_t) s_headerSize);
    for (size_t i = 0; i < blobs.size(); i++)
    {
        uint8_t *entry = &header[(size_t) blobs[i].first * s_entrySize];
        uint32_t offset;
        uint32_t capacity;
        memcpy(&offset, entry, 4);
        memcpy(&capacity, entry + 8, 4);

        uint32_t length = (uint32_t) blobs[i].second.size();
        if (offset == 0 || length > capacity)
        {
            offset   = end;
            capacity = length;
            end += length;
        }

        file.seekp((std::streamoff) offset);
        file.write((const char *) blobs[i].second.data(), (std::streamsize) length);
        memcpy(entry, &offset, 4);
        memcpy(entry + 4, &length, 4);
        memcpy(entry + 8, &capacity, 4);
    }

    file.seekp(0);
    file.write((const char *) header.data(), (std::streamsize) header.size());
    file.flush();
}
