#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkDispatch.h"

#include <stdexcept>

#define EVERSTEAD_VK_LOAD_GLOBAL(name)                                                                                                               \
    name = (PFN_##name) vkGetInstanceProcAddr(nullptr, #name);                                                                                       \
    if (!name)                                                                                                                                       \
    {                                                                                                                                                \
        throw std::runtime_error("missing Vulkan function " #name);                                                                                  \
    }

#define EVERSTEAD_VK_LOAD_INSTANCE(name)                                                                                                             \
    name = (PFN_##name) vkGetInstanceProcAddr(instance, #name);                                                                                      \
    if (!name)                                                                                                                                       \
    {                                                                                                                                                \
        throw std::runtime_error("missing Vulkan function " #name);                                                                                  \
    }

#define EVERSTEAD_VK_LOAD_DEVICE(name)                                                                                                               \
    name = (PFN_##name) vkGetDeviceProcAddr(device, #name);                                                                                          \
    if (!name)                                                                                                                                       \
    {                                                                                                                                                \
        throw std::runtime_error("missing Vulkan function " #name);                                                                                  \
    }

#define EVERSTEAD_VK_LOAD_OPTIONAL_INSTANCE(feature, name)                                                                                           \
    name = nullptr;                                                                                                                                  \
    if (features.feature)                                                                                                                            \
    {                                                                                                                                                \
        EVERSTEAD_VK_LOAD_INSTANCE(name)                                                                                                             \
    }

#define EVERSTEAD_VK_LOAD_OPTIONAL_DEVICE(feature, name)                                                                                             \
    name = nullptr;                                                                                                                                  \
    if (features.feature)                                                                                                                            \
    {                                                                                                                                                \
        EVERSTEAD_VK_LOAD_DEVICE(name)                                                                                                               \
    }

void VkDispatch::LoadGlobal(PFN_vkGetInstanceProcAddr getInstanceProcAddr)
{
    vkGetInstanceProcAddr = getInstanceProcAddr;
    EVERSTEAD_VK_GLOBAL_FUNCTIONS(EVERSTEAD_VK_LOAD_GLOBAL)
}

void VkDispatch::LoadInstance(VkInstance instance, const VkFeatures &features)
{
    EVERSTEAD_VK_INSTANCE_FUNCTIONS(EVERSTEAD_VK_LOAD_INSTANCE)
    EVERSTEAD_VK_OPTIONAL_INSTANCE_FUNCTIONS(EVERSTEAD_VK_LOAD_OPTIONAL_INSTANCE)
}

void VkDispatch::LoadDevice(VkDevice device, const VkFeatures &features)
{
    EVERSTEAD_VK_DEVICE_FUNCTIONS(EVERSTEAD_VK_LOAD_DEVICE)
    EVERSTEAD_VK_OPTIONAL_DEVICE_FUNCTIONS(EVERSTEAD_VK_LOAD_OPTIONAL_DEVICE)
}

#endif
