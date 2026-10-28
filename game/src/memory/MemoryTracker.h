#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

class MemoryTracker
{
public:
    struct Stats
    {
        int textureCount;
        int bufferCount;
        int vertexArrayCount;
        uint64_t trackedAllocatedBytes;
    };

    static MemoryTracker *GetInstance();

    void GenTextures(int count);
    void DeleteTextures(int count);

    void GenBuffers(int count);
    void DeleteBuffers(int count);

    void GenVertexArrays(int count);
    void DeleteVertexArrays(int count);

    std::vector<uint8_t> CreateByteBuffer(size_t count);
    std::vector<int> CreateIntBuffer(size_t count);
    std::vector<float> CreateFloatBuffer(size_t count);

    void TrackAllocation(size_t bytes);
    void TrackFree(size_t bytes);

    Stats GetStats() const;

private:
    MemoryTracker() = default;

    std::atomic<int> m_textureCount{0};
    std::atomic<int> m_bufferCount{0};
    std::atomic<int> m_vertexArrayCount{0};
    std::atomic<uint64_t> m_trackedAllocatedBytes{0};
};
