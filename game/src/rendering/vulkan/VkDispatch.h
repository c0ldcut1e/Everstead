#pragma once

#include <vulkan/vulkan.h>

#include "VkFeatures.h"

#define EVERSTEAD_VK_GLOBAL_FUNCTIONS(X)                                                                                                             \
    X(vkCreateInstance)                                                                                                                              \
    X(vkEnumerateInstanceExtensionProperties)                                                                                                        \
    X(vkEnumerateInstanceLayerProperties)

#define EVERSTEAD_VK_INSTANCE_FUNCTIONS(X)                                                                                                           \
    X(vkDestroyInstance)                                                                                                                             \
    X(vkEnumeratePhysicalDevices)                                                                                                                    \
    X(vkGetPhysicalDeviceProperties)                                                                                                                 \
    X(vkGetPhysicalDeviceProperties2)                                                                                                                \
    X(vkGetPhysicalDeviceFeatures)                                                                                                                   \
    X(vkGetPhysicalDeviceFeatures2)                                                                                                                  \
    X(vkGetPhysicalDeviceQueueFamilyProperties)                                                                                                      \
    X(vkGetPhysicalDeviceMemoryProperties)                                                                                                           \
    X(vkGetPhysicalDeviceMemoryProperties2)                                                                                                          \
    X(vkGetPhysicalDeviceFormatProperties)                                                                                                           \
    X(vkEnumerateDeviceExtensionProperties)                                                                                                          \
    X(vkCreateDevice)                                                                                                                                \
    X(vkGetDeviceProcAddr)                                                                                                                           \
    X(vkDestroySurfaceKHR)                                                                                                                           \
    X(vkGetPhysicalDeviceSurfaceSupportKHR)                                                                                                          \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)                                                                                                     \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR)                                                                                                          \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR)                                                                                                     \
    X(vkGetPhysicalDeviceFormatProperties2)                                                                                                          \
    X(vkGetPhysicalDeviceImageFormatProperties)                                                                                                      \
    X(vkGetPhysicalDeviceQueueFamilyProperties2)

#define EVERSTEAD_VK_DEVICE_FUNCTIONS(X)                                                                                                             \
    X(vkDestroyDevice)                                                                                                                               \
    X(vkGetDeviceQueue)                                                                                                                              \
    X(vkDeviceWaitIdle)                                                                                                                              \
    X(vkQueueSubmit)                                                                                                                                 \
    X(vkCreateSwapchainKHR)                                                                                                                          \
    X(vkDestroySwapchainKHR)                                                                                                                         \
    X(vkGetSwapchainImagesKHR)                                                                                                                       \
    X(vkAcquireNextImageKHR)                                                                                                                         \
    X(vkQueuePresentKHR)                                                                                                                             \
    X(vkAllocateMemory)                                                                                                                              \
    X(vkFreeMemory)                                                                                                                                  \
    X(vkMapMemory)                                                                                                                                   \
    X(vkCreateBuffer)                                                                                                                                \
    X(vkDestroyBuffer)                                                                                                                               \
    X(vkGetBufferMemoryRequirements)                                                                                                                 \
    X(vkBindBufferMemory)                                                                                                                            \
    X(vkCreateImage)                                                                                                                                 \
    X(vkDestroyImage)                                                                                                                                \
    X(vkGetImageMemoryRequirements)                                                                                                                  \
    X(vkBindImageMemory)                                                                                                                             \
    X(vkCreateImageView)                                                                                                                             \
    X(vkDestroyImageView)                                                                                                                            \
    X(vkCreateSampler)                                                                                                                               \
    X(vkDestroySampler)                                                                                                                              \
    X(vkCreateShaderModule)                                                                                                                          \
    X(vkDestroyShaderModule)                                                                                                                         \
    X(vkCreateDescriptorSetLayout)                                                                                                                   \
    X(vkDestroyDescriptorSetLayout)                                                                                                                  \
    X(vkCreatePipelineLayout)                                                                                                                        \
    X(vkDestroyPipelineLayout)                                                                                                                       \
    X(vkCreateGraphicsPipelines)                                                                                                                     \
    X(vkDestroyPipeline)                                                                                                                             \
    X(vkCreatePipelineCache)                                                                                                                         \
    X(vkDestroyPipelineCache)                                                                                                                        \
    X(vkGetPipelineCacheData)                                                                                                                        \
    X(vkCreateRenderPass)                                                                                                                            \
    X(vkDestroyRenderPass)                                                                                                                           \
    X(vkCreateFramebuffer)                                                                                                                           \
    X(vkDestroyFramebuffer)                                                                                                                          \
    X(vkCreateDescriptorPool)                                                                                                                        \
    X(vkDestroyDescriptorPool)                                                                                                                       \
    X(vkResetDescriptorPool)                                                                                                                         \
    X(vkAllocateDescriptorSets)                                                                                                                      \
    X(vkUpdateDescriptorSets)                                                                                                                        \
    X(vkCreateCommandPool)                                                                                                                           \
    X(vkDestroyCommandPool)                                                                                                                          \
    X(vkResetCommandPool)                                                                                                                            \
    X(vkAllocateCommandBuffers)                                                                                                                      \
    X(vkBeginCommandBuffer)                                                                                                                          \
    X(vkEndCommandBuffer)                                                                                                                            \
    X(vkCreateFence)                                                                                                                                 \
    X(vkDestroyFence)                                                                                                                                \
    X(vkWaitForFences)                                                                                                                               \
    X(vkResetFences)                                                                                                                                 \
    X(vkCreateSemaphore)                                                                                                                             \
    X(vkDestroySemaphore)                                                                                                                            \
    X(vkCmdBeginRenderPass)                                                                                                                          \
    X(vkCmdEndRenderPass)                                                                                                                            \
    X(vkCmdBindPipeline)                                                                                                                             \
    X(vkCmdBindVertexBuffers)                                                                                                                        \
    X(vkCmdBindIndexBuffer)                                                                                                                          \
    X(vkCmdBindDescriptorSets)                                                                                                                       \
    X(vkCmdSetViewport)                                                                                                                              \
    X(vkCmdSetScissor)                                                                                                                               \
    X(vkCmdSetDepthBias)                                                                                                                             \
    X(vkCmdSetLineWidth)                                                                                                                             \
    X(vkCmdDraw)                                                                                                                                     \
    X(vkCmdDrawIndexed)                                                                                                                              \
    X(vkCmdDrawIndexedIndirect)                                                                                                                      \
    X(vkCmdClearAttachments)                                                                                                                         \
    X(vkCmdCopyBuffer)                                                                                                                               \
    X(vkCmdCopyBufferToImage)                                                                                                                        \
    X(vkCmdCopyImage)                                                                                                                                \
    X(vkCmdCopyImageToBuffer)                                                                                                                        \
    X(vkCmdBlitImage)                                                                                                                                \
    X(vkCmdPipelineBarrier)                                                                                                                          \
    X(vkQueueWaitIdle)                                                                                                                               \
    X(vkUnmapMemory)                                                                                                                                 \
    X(vkFlushMappedMemoryRanges)                                                                                                                     \
    X(vkInvalidateMappedMemoryRanges)                                                                                                                \
    X(vkGetBufferMemoryRequirements2)                                                                                                                \
    X(vkGetImageMemoryRequirements2)                                                                                                                 \
    X(vkBindBufferMemory2)                                                                                                                           \
    X(vkBindImageMemory2)                                                                                                                            \
    X(vkGetBufferDeviceAddress)                                                                                                                      \
    X(vkCreateBufferView)                                                                                                                            \
    X(vkDestroyBufferView)                                                                                                                           \
    X(vkCreateComputePipelines)                                                                                                                      \
    X(vkFreeDescriptorSets)                                                                                                                          \
    X(vkCreateDescriptorUpdateTemplate)                                                                                                              \
    X(vkDestroyDescriptorUpdateTemplate)                                                                                                             \
    X(vkUpdateDescriptorSetWithTemplate)                                                                                                             \
    X(vkCreateRenderPass2)                                                                                                                           \
    X(vkFreeCommandBuffers)                                                                                                                          \
    X(vkResetCommandBuffer)                                                                                                                          \
    X(vkGetFenceStatus)                                                                                                                              \
    X(vkGetSemaphoreCounterValue)                                                                                                                    \
    X(vkWaitSemaphores)                                                                                                                              \
    X(vkSignalSemaphore)                                                                                                                             \
    X(vkCreateEvent)                                                                                                                                 \
    X(vkDestroyEvent)                                                                                                                                \
    X(vkCreateQueryPool)                                                                                                                             \
    X(vkDestroyQueryPool)                                                                                                                            \
    X(vkGetQueryPoolResults)                                                                                                                         \
    X(vkResetQueryPool)                                                                                                                              \
    X(vkCmdSetEvent)                                                                                                                                 \
    X(vkCmdResetEvent)                                                                                                                               \
    X(vkCmdWaitEvents)                                                                                                                               \
    X(vkCmdResetQueryPool)                                                                                                                           \
    X(vkCmdBeginQuery)                                                                                                                               \
    X(vkCmdEndQuery)                                                                                                                                 \
    X(vkCmdWriteTimestamp)                                                                                                                           \
    X(vkCmdCopyQueryPoolResults)                                                                                                                     \
    X(vkCmdPushConstants)                                                                                                                            \
    X(vkCmdSetBlendConstants)                                                                                                                        \
    X(vkCmdSetDepthBounds)                                                                                                                           \
    X(vkCmdSetStencilCompareMask)                                                                                                                    \
    X(vkCmdSetStencilWriteMask)                                                                                                                      \
    X(vkCmdSetStencilReference)                                                                                                                      \
    X(vkCmdDrawIndirect)                                                                                                                             \
    X(vkCmdDrawIndirectCount)                                                                                                                        \
    X(vkCmdDrawIndexedIndirectCount)                                                                                                                 \
    X(vkCmdDispatch)                                                                                                                                 \
    X(vkCmdDispatchIndirect)                                                                                                                         \
    X(vkCmdDispatchBase)                                                                                                                             \
    X(vkCmdFillBuffer)                                                                                                                               \
    X(vkCmdUpdateBuffer)                                                                                                                             \
    X(vkCmdClearColorImage)                                                                                                                          \
    X(vkCmdClearDepthStencilImage)                                                                                                                   \
    X(vkCmdResolveImage)                                                                                                                             \
    X(vkCmdBeginRenderPass2)                                                                                                                         \
    X(vkCmdNextSubpass)                                                                                                                              \
    X(vkCmdNextSubpass2)                                                                                                                             \
    X(vkCmdEndRenderPass2)                                                                                                                           \
    X(vkCmdExecuteCommands)

// Optional functions are tagged with the VkFeatures flag that loads them and stay null while it is off
#define EVERSTEAD_VK_OPTIONAL_INSTANCE_FUNCTIONS(X)                                                                                                  \
    X(debugUtils, vkCreateDebugUtilsMessengerEXT)                                                                                                    \
    X(debugUtils, vkDestroyDebugUtilsMessengerEXT)                                                                                                   \
    X(debugUtils, vkSetDebugUtilsObjectNameEXT)                                                                                                      \
    X(debugUtils, vkSetDebugUtilsObjectTagEXT)                                                                                                       \
    X(debugUtils, vkCmdBeginDebugUtilsLabelEXT)                                                                                                      \
    X(debugUtils, vkCmdEndDebugUtilsLabelEXT)                                                                                                        \
    X(debugUtils, vkCmdInsertDebugUtilsLabelEXT)                                                                                                     \
    X(debugUtils, vkQueueBeginDebugUtilsLabelEXT)                                                                                                    \
    X(debugUtils, vkQueueEndDebugUtilsLabelEXT)                                                                                                      \
    X(debugUtils, vkQueueInsertDebugUtilsLabelEXT)

#define EVERSTEAD_VK_OPTIONAL_DEVICE_FUNCTIONS(X)                                                                                                    \
    X(dynamicRendering, vkCmdBeginRenderingKHR)                                                                                                      \
    X(dynamicRendering, vkCmdEndRenderingKHR)                                                                                                        \
    X(synchronization2, vkCmdPipelineBarrier2KHR)                                                                                                    \
    X(synchronization2, vkQueueSubmit2KHR)                                                                                                           \
    X(synchronization2, vkCmdWriteTimestamp2KHR)                                                                                                     \
    X(synchronization2, vkCmdSetEvent2KHR)                                                                                                           \
    X(synchronization2, vkCmdResetEvent2KHR)                                                                                                         \
    X(synchronization2, vkCmdWaitEvents2KHR)                                                                                                         \
    X(extendedDynamicState, vkCmdSetCullModeEXT)                                                                                                     \
    X(extendedDynamicState, vkCmdSetFrontFaceEXT)                                                                                                    \
    X(extendedDynamicState, vkCmdSetPrimitiveTopologyEXT)                                                                                            \
    X(extendedDynamicState, vkCmdSetViewportWithCountEXT)                                                                                            \
    X(extendedDynamicState, vkCmdSetScissorWithCountEXT)                                                                                             \
    X(extendedDynamicState, vkCmdBindVertexBuffers2EXT)                                                                                              \
    X(extendedDynamicState, vkCmdSetDepthTestEnableEXT)                                                                                              \
    X(extendedDynamicState, vkCmdSetDepthWriteEnableEXT)                                                                                             \
    X(extendedDynamicState, vkCmdSetDepthCompareOpEXT)                                                                                               \
    X(extendedDynamicState, vkCmdSetDepthBoundsTestEnableEXT)                                                                                        \
    X(extendedDynamicState, vkCmdSetStencilTestEnableEXT)                                                                                            \
    X(extendedDynamicState, vkCmdSetStencilOpEXT)                                                                                                    \
    X(extendedDynamicState2, vkCmdSetRasterizerDiscardEnableEXT)                                                                                     \
    X(extendedDynamicState2, vkCmdSetDepthBiasEnableEXT)                                                                                             \
    X(extendedDynamicState2, vkCmdSetPrimitiveRestartEnableEXT)                                                                                      \
    X(extendedDynamicState2, vkCmdSetLogicOpEXT)                                                                                                     \
    X(extendedDynamicState2, vkCmdSetPatchControlPointsEXT)                                                                                          \
    X(pushDescriptor, vkCmdPushDescriptorSetKHR)                                                                                                     \
    X(pushDescriptor, vkCmdPushDescriptorSetWithTemplateKHR)                                                                                         \
    X(rayQuery, vkCreateAccelerationStructureKHR)                                                                                                    \
    X(rayQuery, vkDestroyAccelerationStructureKHR)                                                                                                   \
    X(rayQuery, vkGetAccelerationStructureBuildSizesKHR)                                                                                             \
    X(rayQuery, vkCmdBuildAccelerationStructuresKHR)                                                                                                 \
    X(rayQuery, vkGetAccelerationStructureDeviceAddressKHR)

#define EVERSTEAD_VK_DECLARE_FUNCTION(name)                   PFN_##name name;
#define EVERSTEAD_VK_DECLARE_OPTIONAL_FUNCTION(feature, name) PFN_##name name;

struct VkDispatch
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    EVERSTEAD_VK_GLOBAL_FUNCTIONS(EVERSTEAD_VK_DECLARE_FUNCTION)
    EVERSTEAD_VK_INSTANCE_FUNCTIONS(EVERSTEAD_VK_DECLARE_FUNCTION)
    EVERSTEAD_VK_DEVICE_FUNCTIONS(EVERSTEAD_VK_DECLARE_FUNCTION)
    EVERSTEAD_VK_OPTIONAL_INSTANCE_FUNCTIONS(EVERSTEAD_VK_DECLARE_OPTIONAL_FUNCTION)
    EVERSTEAD_VK_OPTIONAL_DEVICE_FUNCTIONS(EVERSTEAD_VK_DECLARE_OPTIONAL_FUNCTION)

    void LoadGlobal(PFN_vkGetInstanceProcAddr getInstanceProcAddr);
    void LoadInstance(VkInstance instance, const VkFeatures &features);
    void LoadDevice(VkDevice device, const VkFeatures &features);
};
