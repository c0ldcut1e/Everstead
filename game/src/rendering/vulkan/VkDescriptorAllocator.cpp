#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkDescriptorAllocator.h"

#include <iterator>
#include <stdexcept>

VkDescriptorAllocator::VkDescriptorAllocator()
{
    m_vk                     = nullptr;
    m_device                 = VK_NULL_HANDLE;
    m_accelerationStructures = false;
    m_pool                   = 0;
}

void VkDescriptorAllocator::Init(VkDispatch *vk, VkDevice device, bool accelerationStructures)
{
    m_vk                     = vk;
    m_device                 = device;
    m_accelerationStructures = accelerationStructures;
}

void VkDescriptorAllocator::Shutdown()
{
    for (size_t i = 0; i < m_pools.size(); i++)
    {
        m_vk->vkDestroyDescriptorPool(m_device, m_pools[i], nullptr);
    }

    m_pools.clear();
}

void VkDescriptorAllocator::Reset()
{
    for (size_t i = 0; i < m_pools.size(); i++)
    {
        m_vk->vkResetDescriptorPool(m_device, m_pools[i], 0);
    }

    m_pool = 0;
}

void VkDescriptorAllocator::AddPool()
{
    const uint32_t kSetsPerPool = 1024;

    VkDescriptorPoolSize sizes[] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kSetsPerPool * 2}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kSetsPerPool * 4},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kSetsPerPool / 8},         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kSetsPerPool / 4},
            {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, kSetsPerPool},
    };

    VkDescriptorPoolCreateInfo info = {};
    info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.maxSets                    = kSetsPerPool;
    info.poolSizeCount              = (uint32_t) std::size(sizes) - (m_accelerationStructures ? 0 : 1);
    info.pPoolSizes                 = sizes;

    VkDescriptorPool pool;
    if (m_vk->vkCreateDescriptorPool(m_device, &info, nullptr, &pool) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateDescriptorPool failed");
    }

    m_pools.push_back(pool);
}

VkDescriptorSet VkDescriptorAllocator::Allocate(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo info = {};
    info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorSetCount          = 1;
    info.pSetLayouts                 = &layout;

    VkDescriptorSet set;
    while (true)
    {
        if (m_pool == m_pools.size())
        {
            AddPool();
        }

        info.descriptorPool = m_pools[m_pool];
        VkResult result     = m_vk->vkAllocateDescriptorSets(m_device, &info, &set);
        if (result == VK_SUCCESS)
        {
            return set;
        }

        if (result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL)
        {
            throw std::runtime_error("vkAllocateDescriptorSets failed");
        }

        m_pool++;
    }
}

#endif
