#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkAllocator.h"

#include <stdexcept>

VkAllocator::VkAllocator()
{
    m_vk            = nullptr;
    m_device        = VK_NULL_HANDLE;
    m_deviceAddress = false;
}

void VkAllocator::Init(VkDispatch *vk, VkDevice device, const VkPhysicalDeviceMemoryProperties &properties, bool deviceAddress)
{
    m_vk            = vk;
    m_device        = device;
    m_properties    = properties;
    m_deviceAddress = deviceAddress;
}

void VkAllocator::Shutdown()
{
    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        if (m_blocks[i].memory != VK_NULL_HANDLE)
        {
            m_vk->vkFreeMemory(m_device, m_blocks[i].memory, nullptr);
        }
    }

    m_blocks.clear();
}

uint32_t VkAllocator::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags flags) const
{
    for (uint32_t i = 0; i < m_properties.memoryTypeCount; i++)
    {
        if ((typeBits & (1 << i)) && (m_properties.memoryTypes[i].propertyFlags & flags) == flags)
        {
            return i;
        }
    }

    throw std::runtime_error("no Vulkan memory type matches the request");
}

int VkAllocator::CreateBlock(VkDeviceSize size, uint32_t memoryType, bool linear, bool dedicated)
{
    VkMemoryAllocateInfo info       = {};
    info.sType                      = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize             = size;
    info.memoryTypeIndex            = memoryType;
    VkMemoryAllocateFlagsInfo flags = {};
    flags.sType                     = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flags.flags                     = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    info.pNext                      = m_deviceAddress ? &flags : nullptr;

    Block block;
    if (m_vk->vkAllocateMemory(m_device, &info, nullptr, &block.memory) != VK_SUCCESS)
    {
        throw std::runtime_error("vkAllocateMemory failed");
    }

    block.size       = size;
    block.memoryType = memoryType;
    block.linear     = linear;
    block.dedicated  = dedicated;
    block.mapped     = nullptr;
    block.freeRanges.push_back(Range{0, size});
    if (m_properties.memoryTypes[memoryType].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        m_vk->vkMapMemory(m_device, block.memory, 0, VK_WHOLE_SIZE, 0, (void **) &block.mapped);
    }

    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        if (m_blocks[i].memory == VK_NULL_HANDLE)
        {
            m_blocks[i] = block;
            return (int) i;
        }
    }

    m_blocks.push_back(block);
    return (int) m_blocks.size() - 1;
}

bool VkAllocator::TryAllocate(int blockIndex, const VkMemoryRequirements &requirements, VkAllocation *allocation)
{
    Block &block = m_blocks[blockIndex];
    for (size_t i = 0; i < block.freeRanges.size(); i++)
    {
        Range &range         = block.freeRanges[i];
        VkDeviceSize aligned = (range.offset + requirements.alignment - 1) / requirements.alignment * requirements.alignment;
        VkDeviceSize end     = aligned + requirements.size;
        if (end > range.offset + range.size)
        {
            continue;
        }

        allocation->memory      = block.memory;
        allocation->offset      = aligned;
        allocation->rangeOffset = range.offset;
        allocation->rangeSize   = end - range.offset;
        allocation->mapped      = block.mapped ? block.mapped + aligned : nullptr;
        allocation->block       = blockIndex;

        range.size -= end - range.offset;
        range.offset = end;
        if (range.size == 0)
        {
            block.freeRanges.erase(block.freeRanges.begin() + i);
        }

        return true;
    }

    return false;
}

VkAllocation VkAllocator::Allocate(const VkMemoryRequirements &requirements, VkMemoryPropertyFlags flags, bool linear)
{
    const VkDeviceSize kBlockSize = 64 * 1024 * 1024;

    uint32_t memoryType = FindMemoryType(requirements.memoryTypeBits, flags);
    VkAllocation allocation;
    if (requirements.size > kBlockSize / 2)
    {
        TryAllocate(CreateBlock(requirements.size, memoryType, linear, true), requirements, &allocation);
        return allocation;
    }

    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        const Block &block = m_blocks[i];
        if (block.memory != VK_NULL_HANDLE && !block.dedicated && block.memoryType == memoryType && block.linear == linear &&
            TryAllocate((int) i, requirements, &allocation))
        {
            return allocation;
        }
    }

    TryAllocate(CreateBlock(kBlockSize, memoryType, linear, false), requirements, &allocation);
    return allocation;
}

void VkAllocator::Free(const VkAllocation &allocation)
{
    Block &block = m_blocks[allocation.block];
    if (block.dedicated)
    {
        m_vk->vkFreeMemory(m_device, block.memory, nullptr);
        block.memory = VK_NULL_HANDLE;
        block.freeRanges.clear();
        return;
    }

    std::vector<Range> &ranges = block.freeRanges;
    size_t index               = 0;
    while (index < ranges.size() && ranges[index].offset < allocation.rangeOffset)
    {
        index++;
    }

    ranges.insert(ranges.begin() + index, Range{allocation.rangeOffset, allocation.rangeSize});
    if (index + 1 < ranges.size() && ranges[index].offset + ranges[index].size == ranges[index + 1].offset)
    {
        ranges[index].size += ranges[index + 1].size;
        ranges.erase(ranges.begin() + index + 1);
    }

    if (index > 0 && ranges[index - 1].offset + ranges[index - 1].size == ranges[index].offset)
    {
        ranges[index - 1].size += ranges[index].size;
        ranges.erase(ranges.begin() + index);
    }
}

#endif
