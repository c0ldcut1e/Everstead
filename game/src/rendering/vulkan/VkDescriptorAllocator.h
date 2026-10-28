#pragma once

#include <vector>

#include "VkDispatch.h"

class VkDescriptorAllocator
{
public:
    VkDescriptorAllocator();

    void Init(VkDispatch *vk, VkDevice device, bool accelerationStructures);
    void Shutdown();
    void Reset();

    VkDescriptorSet Allocate(VkDescriptorSetLayout layout);

private:
    void AddPool();

    VkDispatch *m_vk;
    VkDevice m_device;
    bool m_accelerationStructures;
    std::vector<VkDescriptorPool> m_pools;
    size_t m_pool;
};
