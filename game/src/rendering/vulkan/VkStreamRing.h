#pragma once

#include <cstdint>
#include <vector>

#include "VkAllocator.h"
#include "VkDispatch.h"

class VkStreamRing
{
public:
    VkStreamRing();

    void Init(VkDispatch *vk, VkDevice device, VkAllocator *allocator, VkDeviceSize alignment);
    void Shutdown();
    void Reset();

    uint8_t *Allocate(VkDeviceSize size, VkBuffer *buffer, VkDeviceSize *offset);

private:
    struct Chunk
    {
        VkBuffer buffer;
        VkAllocation allocation;
        VkDeviceSize size;
    };

    void AddChunk(VkDeviceSize size);

    VkDispatch *m_vk;
    VkDevice m_device;
    VkAllocator *m_allocator;
    VkDeviceSize m_alignment;
    std::vector<Chunk> m_chunks;
    size_t m_chunk;
    VkDeviceSize m_offset;
};
