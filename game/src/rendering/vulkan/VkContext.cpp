#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkContext.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../core/Logger.h"

#include <SDL2/SDL_vulkan.h>

static VkBool32 VKAPI_CALL DebugMessenger(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                          const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData)
{
    (void) type;
    (void) userData;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        Logger::LogError("Vulkan: %s", data->pMessage);
    }
    else
    {
        Logger::LogWarn("Vulkan: %s", data->pMessage);
    }

    return VK_FALSE;
}

void VkContext::Create(SDL_Window *window)
{
    vk.LoadGlobal((PFN_vkGetInstanceProcAddr) SDL_Vulkan_GetVkGetInstanceProcAddr());

    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    std::vector<const char *> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());

    uint32_t instanceExtensionCount = 0;
    vk.vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
    vk.vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());
    features.debugUtils = false;
    for (size_t i = 0; i < instanceExtensions.size(); i++)
    {
        features.debugUtils |= strcmp(instanceExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
    }

    if (features.debugUtils)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    std::vector<const char *> layers;
    if (getenv("EVERSTEAD_VULKAN_VALIDATION"))
    {
        uint32_t layerCount = 0;
        vk.vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vk.vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        for (size_t i = 0; i < availableLayers.size(); i++)
        {
            if (strcmp(availableLayers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0)
            {
                layers.push_back("VK_LAYER_KHRONOS_validation");
            }
        }

        if (layers.empty())
        {
            Logger::LogWarn("EVERSTEAD_VULKAN_VALIDATION is set but VK_LAYER_KHRONOS_validation is not installed");
        }
    }

    VkApplicationInfo application = {};
    application.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName  = "Everstead";
    application.pEngineName       = "Everstead";
    application.apiVersion        = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo    = {};
    instanceInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo        = &application;
    instanceInfo.enabledExtensionCount   = (uint32_t) extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount       = (uint32_t) layers.size();
    instanceInfo.ppEnabledLayerNames     = layers.data();
    if (vk.vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateInstance failed");
    }

    vk.LoadInstance(instance, features);
    messenger = VK_NULL_HANDLE;
    if (!layers.empty() && features.debugUtils)
    {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
        messengerInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messengerInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerInfo.pfnUserCallback = DebugMessenger;
        if (vk.vkCreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &messenger) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateDebugUtilsMessengerEXT failed");
        }
    }

    if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
    {
        throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
    }

    uint32_t deviceCount = 0;
    vk.vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vk.vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = VK_NULL_HANDLE;
    for (size_t i = 0; i < devices.size(); i++)
    {
        uint32_t family = 0;
        if (!IsSuitable(devices[i], &family))
        {
            continue;
        }

        VkPhysicalDeviceProperties candidate;
        vk.vkGetPhysicalDeviceProperties(devices[i], &candidate);
        if (physicalDevice == VK_NULL_HANDLE ||
            (candidate.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
        {
            physicalDevice = devices[i];
            properties     = candidate;
            queueFamily    = family;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No Vulkan 1.2 device supports what Everstead needs");
    }

    driverProperties                        = {};
    driverProperties.sType                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    VkPhysicalDeviceProperties2 properties2 = {};
    properties2.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties2.pNext                       = &driverProperties;
    vk.vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);
    vk.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    uint32_t deviceExtensionCount = 0;
    vk.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> deviceSupported(deviceExtensionCount);
    vk.vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, deviceSupported.data());

    VkPhysicalDeviceVulkan12Features supported12                           = {};
    supported12.sType                                                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures   = {};
    dynamicRenderingFeatures.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2Features   = {};
    synchronization2Features.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures   = {};
    dynamicStateFeatures.sType                                             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynamicState2Features = {};
    dynamicState2Features.sType                                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
    VkPhysicalDeviceDepthClipControlFeaturesEXT depthClipControlFeatures   = {};
    depthClipControlFeatures.sType                                         = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures  = {};
    accelerationFeatures.sType                                             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures                   = {};
    rayQueryFeatures.sType                                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

    struct OptionalExtension
    {
        const char *name;
        void *featureStruct;
        bool present;
    };

    OptionalExtension optionalExtensions[] = {{VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, nullptr, false},
                                              {VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, nullptr, false},
                                              {VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, &dynamicRenderingFeatures, false},
                                              {VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, &synchronization2Features, false},
                                              {VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, &dynamicStateFeatures, false},
                                              {VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME, &dynamicState2Features, false},
                                              {VK_EXT_DEPTH_CLIP_CONTROL_EXTENSION_NAME, &depthClipControlFeatures, false},
                                              {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, nullptr, false},
                                              {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, &accelerationFeatures, false},
                                              {VK_KHR_RAY_QUERY_EXTENSION_NAME, &rayQueryFeatures, false}};

    // We only chain a feature struct when its extension exists, and vkCreateDevice reuses this chain so every feature it reports gets enabled
    std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    void *extensionChain                       = nullptr;
    for (size_t i = 0; i < std::size(optionalExtensions); i++)
    {
        OptionalExtension &extension = optionalExtensions[i];
        for (size_t j = 0; j < deviceSupported.size() && !extension.present; j++)
        {
            extension.present = strcmp(deviceSupported[j].extensionName, extension.name) == 0;
        }

        if (!extension.present)
        {
            continue;
        }

        deviceExtensions.push_back(extension.name);
        if (extension.featureStruct)
        {
            ((VkBaseOutStructure *) extension.featureStruct)->pNext = (VkBaseOutStructure *) extensionChain;
            extensionChain                                          = extension.featureStruct;
        }
    }

    features.memoryBudget   = optionalExtensions[0].present;
    features.pushDescriptor = optionalExtensions[1].present;

    supported12.pNext                 = extensionChain;
    VkPhysicalDeviceFeatures2 queried = {};
    queried.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    queried.pNext                     = &supported12;
    vk.vkGetPhysicalDeviceFeatures2(physicalDevice, &queried);

    const VkPhysicalDeviceFeatures &supported = queried.features;
    features.wideLines                        = supported.wideLines;
    features.samplerAnisotropy                = supported.samplerAnisotropy;
    features.descriptorIndexing   = supported12.descriptorIndexing && supported12.runtimeDescriptorArray &&
                                    supported12.descriptorBindingPartiallyBound && supported12.descriptorBindingVariableDescriptorCount &&
                                    supported12.shaderSampledImageArrayNonUniformIndexing && supported12.descriptorBindingSampledImageUpdateAfterBind;
    features.timelineSemaphore    = supported12.timelineSemaphore;
    features.bufferDeviceAddress  = supported12.bufferDeviceAddress;
    features.drawIndirectCount    = supported12.drawIndirectCount;
    features.dynamicRendering     = dynamicRenderingFeatures.dynamicRendering;
    features.synchronization2     = synchronization2Features.synchronization2;
    features.extendedDynamicState = dynamicStateFeatures.extendedDynamicState;
    features.extendedDynamicState2 = dynamicState2Features.extendedDynamicState2;
    features.depthClipControl      = depthClipControlFeatures.depthClipControl;
    features.rayQuery =
            optionalExtensions[7].present && accelerationFeatures.accelerationStructure && rayQueryFeatures.rayQuery && features.bufferDeviceAddress;
    accelerationFeatures.accelerationStructure = features.rayQuery;
    rayQueryFeatures.rayQuery                  = features.rayQuery;

    VkPhysicalDeviceVulkan12Features enabled12             = {};
    enabled12.sType                                        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enabled12.pNext                                        = extensionChain;
    enabled12.descriptorIndexing                           = features.descriptorIndexing;
    enabled12.runtimeDescriptorArray                       = features.descriptorIndexing;
    enabled12.descriptorBindingPartiallyBound              = features.descriptorIndexing;
    enabled12.descriptorBindingVariableDescriptorCount     = features.descriptorIndexing;
    enabled12.shaderSampledImageArrayNonUniformIndexing    = features.descriptorIndexing;
    enabled12.descriptorBindingSampledImageUpdateAfterBind = features.descriptorIndexing;
    enabled12.timelineSemaphore                            = features.timelineSemaphore;
    enabled12.bufferDeviceAddress                          = features.bufferDeviceAddress;
    enabled12.drawIndirectCount                            = features.drawIndirectCount;

    VkPhysicalDeviceFeatures2 enabledFeatures          = {};
    enabledFeatures.sType                              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enabledFeatures.pNext                              = &enabled12;
    enabledFeatures.features.fillModeNonSolid          = VK_TRUE;
    enabledFeatures.features.logicOp                   = VK_TRUE;
    enabledFeatures.features.wideLines                 = supported.wideLines;
    enabledFeatures.features.samplerAnisotropy         = supported.samplerAnisotropy;
    enabledFeatures.features.multiDrawIndirect         = VK_TRUE;
    enabledFeatures.features.drawIndirectFirstInstance = VK_TRUE;

    uint32_t familyCount = 0;
    vk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vk.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());
    timestampValidBits = families[queueFamily].timestampValidBits;
    computeFamily      = queueFamily;
    transferFamily     = queueFamily;
    for (uint32_t i = 0; i < familyCount; i++)
    {
        VkQueueFlags flags = families[i].queueFlags;
        if ((flags & VK_QUEUE_COMPUTE_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT) && computeFamily == queueFamily)
        {
            computeFamily = i;
        }

        if ((flags & VK_QUEUE_TRANSFER_BIT) && !(flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) && transferFamily == queueFamily)
        {
            transferFamily = i;
        }
    }

    float priority                        = 1.0f;
    VkDeviceQueueCreateInfo queueInfos[3] = {};
    uint32_t queueInfoCount               = 0;
    const uint32_t kFamilies[]            = {queueFamily, computeFamily, transferFamily};
    for (size_t i = 0; i < std::size(kFamilies); i++)
    {
        if (i > 0 && kFamilies[i] == queueFamily)
        {
            continue;
        }

        queueInfos[queueInfoCount].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[queueInfoCount].queueFamilyIndex = kFamilies[i];
        queueInfos[queueInfoCount].queueCount       = 1;
        queueInfos[queueInfoCount].pQueuePriorities = &priority;
        queueInfoCount++;
    }

    VkDeviceCreateInfo deviceInfo      = {};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext                   = &enabledFeatures;
    deviceInfo.queueCreateInfoCount    = queueInfoCount;
    deviceInfo.pQueueCreateInfos       = queueInfos;
    deviceInfo.enabledExtensionCount   = (uint32_t) deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if (vk.vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateDevice failed");
    }

    vk.LoadDevice(device, features);
    vk.vkGetDeviceQueue(device, queueFamily, 0, &queue);
    vk.vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
    vk.vkGetDeviceQueue(device, transferFamily, 0, &transferQueue);
    Logger::LogInfo("Vulkan device: %s (%s %s), API %u.%u.%u", properties.deviceName, driverProperties.driverName, driverProperties.driverInfo,
                    VK_API_VERSION_MAJOR(properties.apiVersion), VK_API_VERSION_MINOR(properties.apiVersion),
                    VK_API_VERSION_PATCH(properties.apiVersion));
    Logger::LogInfo("Vulkan limits: max texture %u, max draw indirect count %u, min uniform alignment %llu, max anisotropy %.1f",
                    properties.limits.maxImageDimension2D, properties.limits.maxDrawIndirectCount,
                    (unsigned long long) properties.limits.minUniformBufferOffsetAlignment, properties.limits.maxSamplerAnisotropy);
    Logger::LogInfo("Vulkan features: wide lines %d, anisotropy %d, memory budget %d, debug utils %d, descriptor indexing %d, timeline semaphore %d, "
                    "buffer device address %d, draw indirect count %d, dynamic rendering %d, synchronization2 %d, extended dynamic state %d/%d, "
                    "push descriptor %d, depth clip control %d, ray query %d",
                    (int) features.wideLines, (int) features.samplerAnisotropy, (int) features.memoryBudget, (int) features.debugUtils,
                    (int) features.descriptorIndexing, (int) features.timelineSemaphore, (int) features.bufferDeviceAddress,
                    (int) features.drawIndirectCount, (int) features.dynamicRendering, (int) features.synchronization2,
                    (int) features.extendedDynamicState, (int) features.extendedDynamicState2, (int) features.pushDescriptor,
                    (int) features.depthClipControl, (int) features.rayQuery);
    Logger::LogInfo("Vulkan queues: graphics %u, compute %u, transfer %u", queueFamily, computeFamily, transferFamily);
}

bool VkContext::IsSuitable(VkPhysicalDevice candidate, uint32_t *family)
{
    VkPhysicalDeviceProperties candidateProperties;
    vk.vkGetPhysicalDeviceProperties(candidate, &candidateProperties);
    if (candidateProperties.apiVersion < VK_API_VERSION_1_2)
    {
        return false;
    }

    uint32_t extensionCount = 0;
    vk.vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vk.vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, extensions.data());

    bool swapchain = false;
    for (size_t i = 0; i < extensions.size(); i++)
    {
        swapchain |= strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
    }

    if (!swapchain)
    {
        return false;
    }

    VkPhysicalDeviceFeatures2 candidateFeatures = {};
    candidateFeatures.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vk.vkGetPhysicalDeviceFeatures2(candidate, &candidateFeatures);
    if (!candidateFeatures.features.fillModeNonSolid || !candidateFeatures.features.logicOp || !candidateFeatures.features.multiDrawIndirect ||
        !candidateFeatures.features.drawIndirectFirstInstance)
    {
        return false;
    }

    const VkFormat kVertexFormats[] = {VK_FORMAT_R16G16B16A16_USCALED, VK_FORMAT_R16G16_SSCALED};
    for (size_t i = 0; i < 2; i++)
    {
        VkFormatProperties formatProperties;
        vk.vkGetPhysicalDeviceFormatProperties(candidate, kVertexFormats[i], &formatProperties);
        if (!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT))
        {
            return false;
        }
    }

    uint32_t familyCount = 0;
    vk.vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vk.vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
    for (uint32_t i = 0; i < familyCount; i++)
    {
        VkBool32 present = VK_FALSE;
        vk.vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present);
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
        {
            *family = i;
            return true;
        }
    }

    return false;
}

void VkContext::SetObjectName(VkObjectType type, uint64_t handle, const char *format, ...) const
{
    if (!features.debugUtils)
    {
        return;
    }

    char name[128];
    va_list args;
    va_start(args, format);
    vsnprintf(name, sizeof(name), format, args);
    va_end(args);

    VkDebugUtilsObjectNameInfoEXT info = {};
    info.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType                    = type;
    info.objectHandle                  = handle;
    info.pObjectName                   = name;
    vk.vkSetDebugUtilsObjectNameEXT(device, &info);
}

void VkContext::Destroy()
{
    vk.vkDestroyDevice(device, nullptr);
    vk.vkDestroySurfaceKHR(instance, surface, nullptr);
    if (messenger != VK_NULL_HANDLE)
    {
        vk.vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    }

    vk.vkDestroyInstance(instance, nullptr);
}

#endif
