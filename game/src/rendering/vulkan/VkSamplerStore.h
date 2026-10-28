#pragma once

#include <cstdint>
#include <unordered_map>

#include "../RenderTypes.h"
#include "VkDispatch.h"

struct VkSamplerKey
{
    eTEXTURE_FILTER minFilter;
    eTEXTURE_FILTER magFilter;
    eTEXTURE_WRAP wrapS;
    eTEXTURE_WRAP wrapT;
    uint16_t maxLevel;
    float anisotropy;
};

class VkSamplerStore
{
public:
    VkSamplerStore();

    void Init(VkDispatch *vk, VkDevice device, bool anisotropySupported);
    void Shutdown();

    VkSampler Get(const VkSamplerKey &key);

private:
    VkDispatch *m_vk;
    VkDevice m_device;
    bool m_anisotropySupported;
    std::unordered_map<uint64_t, VkSampler> m_samplers;
};
