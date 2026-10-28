#pragma once

#include <cstdint>

#include "VkDispatch.h"
#include "VkFeatures.h"

#include <SDL2/SDL.h>

struct VkContext
{
    VkDispatch vk;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    uint32_t queueFamily;
    uint32_t timestampValidBits;
    VkQueue computeQueue;
    uint32_t computeFamily;
    VkQueue transferQueue;
    uint32_t transferFamily;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceDriverProperties driverProperties;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkFeatures features;
    VkDebugUtilsMessengerEXT messenger;

    void Create(SDL_Window *window);
    void Destroy();
    void SetObjectName(VkObjectType type, uint64_t handle, const char *format, ...) const;

private:
    bool IsSuitable(VkPhysicalDevice candidate, uint32_t *family);
};
