#pragma once

#include <cstdint>
#include <vector>

#include "VkDispatch.h"

struct VkAllocation
{
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize rangeOffset;
    VkDeviceSize rangeSize;
    uint8_t *mapped;
    int block;
};

class VkAllocator
{
public:
    VkAllocator();

    void Init(VkDispatch *vk, VkDevice device, const VkPhysicalDeviceMemoryProperties &properties, bool deviceAddress);
    void Shutdown();

    VkAllocation Allocate(const VkMemoryRequirements &requirements, VkMemoryPropertyFlags flags, bool linear);
    void Free(const VkAllocation &allocation);

private:
    struct Range
    {
        VkDeviceSize offset;
        VkDeviceSize size;
    };

    struct Block
    {
        VkDeviceMemory memory;
        VkDeviceSize size;
        uint32_t memoryType;
        bool linear;
        bool dedicated;
        uint8_t *mapped;
        std::vector<Range> freeRanges;
    };

    uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags flags) const;
    int CreateBlock(VkDeviceSize size, uint32_t memoryType, bool linear, bool dedicated);
    bool TryAllocate(int blockIndex, const VkMemoryRequirements &requirements, VkAllocation *allocation);

    VkDispatch *m_vk;
    VkDevice m_device;
    VkPhysicalDeviceMemoryProperties m_properties;
    bool m_deviceAddress;
    std::vector<Block> m_blocks;
};
