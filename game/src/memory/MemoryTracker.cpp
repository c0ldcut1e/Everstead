#include "MemoryTracker.h"

MemoryTracker *MemoryTracker::GetInstance()
{
    static MemoryTracker instance;
    return &instance;
}

void MemoryTracker::GenTextures(int count)
{
    if (count > 0)
    {
        m_textureCount.fetch_add(count);
    }
}

void MemoryTracker::DeleteTextures(int count)
{
    if (count > 0)
    {
        m_textureCount.fetch_sub(count);
    }
}

void MemoryTracker::GenBuffers(int count)
{
    if (count > 0)
    {
        m_bufferCount.fetch_add(count);
    }
}

void MemoryTracker::DeleteBuffers(int count)
{
    if (count > 0)
    {
        m_bufferCount.fetch_sub(count);
    }
}

void MemoryTracker::GenVertexArrays(int count)
{
    if (count > 0)
    {
        m_vertexArrayCount.fetch_add(count);
    }
}

void MemoryTracker::DeleteVertexArrays(int count)
{
    if (count > 0)
    {
        m_vertexArrayCount.fetch_sub(count);
    }
}

std::vector<uint8_t> MemoryTracker::CreateByteBuffer(size_t count)
{
    TrackAllocation(count);
    return std::vector<uint8_t>(count);
}

std::vector<int> MemoryTracker::CreateIntBuffer(size_t count)
{
    TrackAllocation(count * sizeof(int));
    return std::vector<int>(count);
}

std::vector<float> MemoryTracker::CreateFloatBuffer(size_t count)
{
    TrackAllocation(count * sizeof(float));
    return std::vector<float>(count);
}

void MemoryTracker::TrackAllocation(size_t bytes)
{
    if (bytes > 0)
    {
        m_trackedAllocatedBytes.fetch_add((uint64_t) bytes);
    }
}

void MemoryTracker::TrackFree(size_t bytes)
{
    if (bytes > 0)
    {
        m_trackedAllocatedBytes.fetch_sub((uint64_t) bytes);
    }
}

MemoryTracker::Stats MemoryTracker::GetStats() const
{
    return Stats{m_textureCount.load(), m_bufferCount.load(), m_vertexArrayCount.load(), m_trackedAllocatedBytes.load()};
}
