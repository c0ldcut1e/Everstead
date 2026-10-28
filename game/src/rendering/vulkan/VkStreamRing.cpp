#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkStreamRing.h"

#include <algorithm>
#include <stdexcept>

VkStreamRing::VkStreamRing()
{
    m_vk        = nullptr;
    m_device    = VK_NULL_HANDLE;
    m_allocator = nullptr;
    m_alignment = 1;
    m_chunk     = 0;
    m_offset    = 0;
}

void VkStreamRing::Init(VkDispatch *vk, VkDevice device, VkAllocator *allocator, VkDeviceSize alignment)
{
    m_vk        = vk;
    m_device    = device;
    m_allocator = allocator;
    m_alignment = alignment;
}

void VkStreamRing::Shutdown()
{
    for (size_t i = 0; i < m_chunks.size(); i++)
    {
        m_vk->vkDestroyBuffer(m_device, m_chunks[i].buffer, nullptr);
        m_allocator->Free(m_chunks[i].allocation);
    }

    m_chunks.clear();
}

void VkStreamRing::Reset()
{
    m_chunk  = 0;
    m_offset = 0;
}

void VkStreamRing::AddChunk(VkDeviceSize size)
{
    VkBufferCreateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size               = size;
    info.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    Chunk chunk;
    chunk.size = size;
    if (m_vk->vkCreateBuffer(m_device, &info, nullptr, &chunk.buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateBuffer failed for the stream ring");
    }

    VkMemoryRequirements requirements;
    m_vk->vkGetBufferMemoryRequirements(m_device, chunk.buffer, &requirements);
    chunk.allocation = m_allocator->Allocate(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
    m_vk->vkBindBufferMemory(m_device, chunk.buffer, chunk.allocation.memory, chunk.allocation.offset);
    m_chunks.push_back(chunk);
}

uint8_t *VkStreamRing::Allocate(VkDeviceSize size, VkBuffer *buffer, VkDeviceSize *offset)
{
    const VkDeviceSize kChunkSize = 4 * 1024 * 1024;

    VkDeviceSize aligned = (m_offset + m_alignment - 1) / m_alignment * m_alignment;
    while (m_chunk < m_chunks.size() && aligned + size > m_chunks[m_chunk].size)
    {
        m_chunk++;
        aligned = 0;
    }

    if (m_chunk == m_chunks.size())
    {
        AddChunk(std::max(size, kChunkSize));
        aligned = 0;
    }

    m_offset = aligned + size;
    *buffer  = m_chunks[m_chunk].buffer;
    *offset  = aligned;
    return m_chunks[m_chunk].allocation.mapped + aligned;
}

#endif
