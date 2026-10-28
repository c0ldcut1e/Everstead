#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkSamplerStore.h"

#include <stdexcept>

VkSamplerStore::VkSamplerStore()
{
    m_vk                  = nullptr;
    m_device              = VK_NULL_HANDLE;
    m_anisotropySupported = false;
}

void VkSamplerStore::Init(VkDispatch *vk, VkDevice device, bool anisotropySupported)
{
    m_vk                  = vk;
    m_device              = device;
    m_anisotropySupported = anisotropySupported;
}

void VkSamplerStore::Shutdown()
{
    for (std::pair<const uint64_t, VkSampler> &entry : m_samplers)
    {
        m_vk->vkDestroySampler(m_device, entry.second, nullptr);
    }

    m_samplers.clear();
}

VkSampler VkSamplerStore::Get(const VkSamplerKey &key)
{
    uint64_t hash = (uint64_t) key.minFilter | ((uint64_t) key.magFilter << 8) | ((uint64_t) key.wrapS << 16) | ((uint64_t) key.wrapT << 24) |
                    ((uint64_t) key.maxLevel << 32) | ((uint64_t) (key.anisotropy * 16.0f) << 48);
    auto it       = m_samplers.find(hash);
    if (it != m_samplers.end())
    {
        return it->second;
    }

    const VkFilter kFilters[]                  = {VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_FILTER_NEAREST};
    const VkSamplerAddressMode kAddressModes[] = {VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};

    bool mipmapped           = key.minFilter == eTextureFilter_NearestMipmapNearest;
    VkSamplerCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter           = kFilters[key.magFilter];
    info.minFilter           = kFilters[key.minFilter];
    info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    info.addressModeU        = kAddressModes[key.wrapS];
    info.addressModeV        = kAddressModes[key.wrapT];
    info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable    = m_anisotropySupported && key.anisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy       = key.anisotropy;
    info.minLod              = 0.0f;
    info.maxLod              = mipmapped ? (float) key.maxLevel : 0.0f;
    info.borderColor         = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    VkSampler sampler;
    if (m_vk->vkCreateSampler(m_device, &info, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateSampler failed");
    }

    m_samplers[hash] = sampler;
    return sampler;
}

#endif
