#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkRenderDevice.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

#include "../../core/FrameProfiler.h"
#include "../../core/Logger.h"
#include "../../io/File.h"
#include "../../memory/MemoryTracker.h"
#include "../../ui/imgui/imgui_impl_sdl2.h"
#include "../../ui/imgui/imgui_impl_vulkan.h"

#include <SDL2/SDL_vulkan.h>
#include <nlohmann/json.hpp>

static const VkFormat kTextureFormats[]           = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                     VK_FORMAT_D32_SFLOAT, VK_FORMAT_R32_UINT};
static const VkPipelineStageFlags kGraphicsStages = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
                                                    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
static const VkAccessFlags kGraphicsAccess = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                                             VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
static const uint32_t kBytesPerPixel[]     = {4, 8, 16, 4, 4};
static const uint32_t kAttribTypeSizes[]   = {4, 2, 2, 1};

static const VkFormat kVertexFormats[4][2][4] = {
        {
                {VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
                {VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT},
        },
        {
                {VK_FORMAT_R16_USCALED, VK_FORMAT_R16G16_USCALED, VK_FORMAT_R16G16B16_USCALED, VK_FORMAT_R16G16B16A16_USCALED},
                {VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16A16_UNORM},
        },
        {
                {VK_FORMAT_R16_SSCALED, VK_FORMAT_R16G16_SSCALED, VK_FORMAT_R16G16B16_SSCALED, VK_FORMAT_R16G16B16A16_SSCALED},
                {VK_FORMAT_R16_SNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16B16_SNORM, VK_FORMAT_R16G16B16A16_SNORM},
        },
        {
                {VK_FORMAT_R8_USCALED, VK_FORMAT_R8G8_USCALED, VK_FORMAT_R8G8B8_USCALED, VK_FORMAT_R8G8B8A8_USCALED},
                {VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM},
        },
};

static PFN_vkVoidFunction LoadImGuiFunction(const char *name, void *userData)
{
    VkContext *context = (VkContext *) userData;
    return context->vk.vkGetInstanceProcAddr(context->instance, name);
}

VkRenderDevice::VkRenderDevice()
{
    m_frameIndex      = 0;
    m_frameNumber     = 0;
    m_nextId          = 1;
    m_dummyBuffer     = {};
    m_readbacks[0]    = {};
    m_readbacks[1]    = {};
    m_readbackTarget  = 1;
    m_readbackPending = false;
    m_readbackSubmit  = 0;
    m_submitNumber    = 1;
    m_timeline        = VK_NULL_HANDLE;
    m_uploadEpoch     = 1;
    m_pendingStages   = 0;
    m_pendingAccess   = 0;
    m_backbufferColor = 0;
    m_backbufferDepth = 0;
    m_arrayBuffer     = 0;
    m_vertexArray     = 0;
    m_program         = 0;
    m_framebuffer     = 0;
    m_inRenderPass    = false;
    m_dynamicDirty    = true;
    m_uniformsBound   = false;
    m_samplersDirty   = true;
    m_boundPipeline   = VK_NULL_HANDLE;
    m_viewport        = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    m_scissor         = {};
    m_scissorTest     = false;
    m_clearColor[0]   = 0.0f;
    m_clearColor[1]   = 0.0f;
    m_clearColor[2]   = 0.0f;
    m_clearColor[3]   = 0.0f;
    m_clearDepth      = 1.0f;
    m_depthBiasFactor = 0.0f;
    m_depthBiasUnits  = 0.0f;
    m_lineWidth       = 1.0f;
    m_vsync           = true;
    m_imguiPending    = false;
    m_swapchainWidth  = 0;
    m_swapchainHeight = 0;

    memset(&m_state, 0, sizeof(m_state));
    m_state.depthFunc  = eCompareFunc_Less;
    m_state.srcFactor  = eBlendFactor_One;
    m_state.dstFactor  = eBlendFactor_Zero;
    m_state.colorMask  = 0xF;
    m_state.depthWrite = 1;

    memset(m_textureBindings, 0, sizeof(m_textureBindings));
    m_vertexArrays[0] = {};
}

VkRenderDevice::~VkRenderDevice()
{
    VkDispatch &vk  = m_context.vk;
    VkDevice device = m_context.device;
    vk.vkDeviceWaitIdle(device);

    for (std::pair<const uint32_t, VkTextureData> &entry : m_textures)
    {
        vk.vkDestroyImageView(device, entry.second.view, nullptr);
        vk.vkDestroyImage(device, entry.second.image, nullptr);
        m_allocator.Free(entry.second.allocation);
    }

    for (std::pair<const uint32_t, VkBufferData> &entry : m_buffers)
    {
        if (entry.second.buffer)
        {
            vk.vkDestroyBuffer(device, entry.second.buffer, nullptr);
            m_allocator.Free(entry.second.allocation);
        }
    }

    for (std::pair<const uint32_t, VkProgramData> &entry : m_programs)
    {
        vk.vkDestroyShaderModule(device, entry.second.vertex, nullptr);
        vk.vkDestroyShaderModule(device, entry.second.fragment, nullptr);
        vk.vkDestroyPipelineLayout(device, entry.second.pipelineLayout, nullptr);
        vk.vkDestroyDescriptorSetLayout(device, entry.second.uniformLayout, nullptr);
        vk.vkDestroyDescriptorSetLayout(device, entry.second.samplerLayout, nullptr);
    }

    for (std::pair<const uint32_t, VkComputeProgramData> &entry : m_computePrograms)
    {
        vk.vkDestroyPipeline(device, entry.second.pipeline, nullptr);
        vk.vkDestroyShaderModule(device, entry.second.module, nullptr);
        vk.vkDestroyPipelineLayout(device, entry.second.pipelineLayout, nullptr);
        vk.vkDestroyDescriptorSetLayout(device, entry.second.layout, nullptr);
    }

    for (std::pair<const uint32_t, VkFramebufferData> &entry : m_framebuffers)
    {
        vk.vkDestroyFramebuffer(device, entry.second.framebuffer, nullptr);
    }

    vk.vkDestroyBuffer(device, m_dummyBuffer.buffer, nullptr);
    m_allocator.Free(m_dummyBuffer.allocation);
    for (int i = 0; i < 2; i++)
    {
        if (m_readbacks[i].buffer)
        {
            vk.vkDestroyBuffer(device, m_readbacks[i].buffer, nullptr);
            m_allocator.Free(m_readbacks[i].allocation);
        }
    }

    for (int i = 0; i < 2; i++)
    {
        Frame &frame = m_frames[i];
        frame.deletions.Flush(&vk, device, &m_allocator);
        frame.ring.Shutdown();
        frame.descriptors.Shutdown();
        vk.vkDestroyCommandPool(device, frame.pool, nullptr);
        vk.vkDestroySemaphore(device, frame.imageAvailable, nullptr);
        vk.vkDestroyQueryPool(device, frame.timers, nullptr);
    }

    vk.vkDestroySemaphore(device, m_timeline, nullptr);

    m_pipelines.Shutdown();
    m_samplers.Shutdown();
    m_swapchain.Shutdown();
    if (m_context.features.rayQuery)
    {
        m_rayScene.Shutdown();
    }

    m_allocator.Shutdown();
    m_context.Destroy();
}

eRENDER_BACKEND VkRenderDevice::GetBackend() const
{
    return eRenderBackend_Vulkan;
}

uint32_t VkRenderDevice::GetWindowFlags() const
{
    return SDL_WINDOW_VULKAN;
}

const VkContext &VkRenderDevice::GetContext() const
{
    return m_context;
}

VkAllocator &VkRenderDevice::GetAllocator()
{
    return m_allocator;
}

// Anything pushed here is destroyed once the GPU finishes the frame being recorded
VkDeletionQueue &VkRenderDevice::GetDeletionQueue()
{
    return m_frames[m_frameIndex].deletions;
}

// This marks the buffer as used by the current submit, so deleting or restaging it waits for the GPU
VkBufferData &VkRenderDevice::UseBuffer(BufferHandle handle)
{
    VkBufferData &buffer = m_buffers[handle.id];
    buffer.usage.submit  = m_submitNumber;
    return buffer;
}

VkTextureData &VkRenderDevice::UseTexture(TextureHandle handle)
{
    VkTextureData &texture = m_textures[handle.id];
    texture.usage.submit   = m_submitNumber;
    return texture;
}

// The returned command buffer is outside any render pass and must be closed with EndExternalCommands before the next draw
VkCommandBuffer VkRenderDevice::BeginExternalCommands(VkPipelineStageFlags dstStages, VkAccessFlags dstAccess)
{
    EndRenderPass();
    MainBarrier(dstStages, dstAccess);
    return m_frames[m_frameIndex].cmd;
}

void VkRenderDevice::EndExternalCommands(VkPipelineStageFlags srcStages, VkAccessFlags srcAccess)
{
    m_pendingStages |= srcStages;
    m_pendingAccess |= srcAccess;
}

void VkRenderDevice::CreateContext(SDL_Window *window)
{
    m_context.Create(window);
    VkDispatch *vk  = &m_context.vk;
    VkDevice device = m_context.device;

    m_allocator.Init(vk, device, m_context.memoryProperties, m_context.features.rayQuery);
    m_pipelines.Init(vk, device, m_context.features);
    m_samplers.Init(vk, device, m_context.features.samplerAnisotropy);

    int width  = 0;
    int height = 0;
    GetDrawableSize(window, &width, &height);
    m_swapchain.Init(vk, m_context.physicalDevice, device, m_context.surface);
    m_swapchain.Create(width, height, m_vsync);
    m_swapchainWidth  = width;
    m_swapchainHeight = height;

    // The timeline counts submits, so a frame is free once it reaches the submit number that frame last used
    VkSemaphoreTypeCreateInfo timelineType = {};
    timelineType.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineType.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineType.initialValue              = 0;
    VkSemaphoreCreateInfo timelineInfo     = {};
    timelineInfo.sType                     = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    timelineInfo.pNext                     = &timelineType;
    if (vk->vkCreateSemaphore(device, &timelineInfo, nullptr, &m_timeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create the timeline semaphore");
    }

    m_context.SetObjectName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t) m_timeline, "submit timeline");

    VkDeviceSize alignment = std::max(m_context.properties.limits.minUniformBufferOffsetAlignment, (VkDeviceSize) 16);
    for (int i = 0; i < 2; i++)
    {
        Frame &frame = m_frames[i];

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex        = m_context.queueFamily;
        if (vk->vkCreateCommandPool(device, &poolInfo, nullptr, &frame.pool) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateCommandPool failed");
        }

        VkCommandBufferAllocateInfo cmdInfo = {};
        cmdInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdInfo.commandPool                 = frame.pool;
        cmdInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdInfo.commandBufferCount          = 2;
        VkCommandBuffer commandBuffers[2];
        if (vk->vkAllocateCommandBuffers(device, &cmdInfo, commandBuffers) != VK_SUCCESS)
        {
            throw std::runtime_error("vkAllocateCommandBuffers failed");
        }

        frame.cmd       = commandBuffers[0];
        frame.uploadCmd = commandBuffers[1];
        m_context.SetObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) frame.cmd, "frame %d commands", i);
        m_context.SetObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) frame.uploadCmd, "frame %d uploads", i);
        frame.uploading = false;
        frame.submit    = 0;
        frame.timing    = false;

        VkQueryPoolCreateInfo queryInfo = {};
        queryInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryInfo.queryType             = VK_QUERY_TYPE_TIMESTAMP;
        queryInfo.queryCount            = kMaxGpuTimers * 2;
        if (vk->vkCreateQueryPool(device, &queryInfo, nullptr, &frame.timers) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateQueryPool failed");
        }

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vk->vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.imageAvailable) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create frame sync objects");
        }

        frame.ring.Init(vk, device, &m_allocator, alignment);
        frame.descriptors.Init(vk, device, m_context.features.rayQuery);
    }

    if (m_context.features.rayQuery)
    {
        m_rayScene.Init(&m_context, &m_allocator);
    }

    BeginFrame();

    VkBufferCreateInfo dummyInfo = {};
    dummyInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    dummyInfo.size               = 16;
    dummyInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    dummyInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
    if (vk->vkCreateBuffer(device, &dummyInfo, nullptr, &m_dummyBuffer.buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateBuffer failed for the dummy vertex buffer");
    }

    m_context.SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t) m_dummyBuffer.buffer, "dummy vertex buffer");

    VkMemoryRequirements requirements;
    vk->vkGetBufferMemoryRequirements(device, m_dummyBuffer.buffer, &requirements);
    m_dummyBuffer.allocation = m_allocator.Allocate(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
    vk->vkBindBufferMemory(device, m_dummyBuffer.buffer, m_dummyBuffer.allocation.memory, m_dummyBuffer.allocation.offset);
    const float kDummyAttrib[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    StageBuffer(&m_dummyBuffer, 0, kDummyAttrib, sizeof(kDummyAttrib));

    m_textures[0]                  = {};
    const uint8_t kDefaultPixel[4] = {0, 0, 0, 255};
    CreateImage(&m_textures[0], 1, 1, eTextureFormat_RGBA8, 1);
    StageImage(&m_textures[0], 0, 0, 0, 1, 1, kDefaultPixel);

    m_backbufferColor = CreateTextureData();
    m_backbufferDepth = CreateTextureData();
    m_framebuffers[0] = {m_backbufferColor, m_backbufferDepth, 0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE};
    ResizeBackbuffer(width, height);

    uint32_t api = m_context.properties.apiVersion;
    m_version    = "Vulkan " + std::to_string(VK_API_VERSION_MAJOR(api)) + "." + std::to_string(VK_API_VERSION_MINOR(api)) + "." +
                   std::to_string(VK_API_VERSION_PATCH(api));
}

void VkRenderDevice::Present(SDL_Window *window)
{
    VkDispatch &vk = m_context.vk;
    Frame &frame   = m_frames[m_frameIndex];

    int width  = 0;
    int height = 0;
    GetDrawableSize(window, &width, &height);
    EndRenderPass();
    EndGpuTimer();

    if (width == 0 || height == 0)
    {
        m_imguiPending = false;
        Submit(VK_NULL_HANDLE, VK_NULL_HANDLE);
        m_frameIndex = (m_frameIndex + 1) % 2;
        BeginFrame();
        return;
    }

    uint32_t imageIndex    = 0;
    VkResult acquireResult = m_swapchain.Acquire(frame.imageAvailable, &imageIndex);
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
    {
        Logger::LogWarn("vkAcquireNextImageKHR returned %d", (int) acquireResult);
        m_imguiPending = false;
        Submit(VK_NULL_HANDLE, VK_NULL_HANDLE);
        m_frameIndex = (m_frameIndex + 1) % 2;
        BeginFrame();
        RecreateSwapchain(width, height);
        return;
    }

    VkExtent2D extent     = m_swapchain.GetExtent();
    VkTextureData &source = m_textures[m_backbufferColor];

    MainBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    TransitionImage(frame.cmd, &source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
                    VK_ACCESS_2_TRANSFER_READ_BIT_KHR);

    VkImageMemoryBarrier imageBarrier        = {};
    imageBarrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.srcAccessMask               = 0;
    imageBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image                       = m_swapchain.GetImage(imageIndex);
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.layerCount = 1;
    vk.vkCmdPipelineBarrier(frame.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

    // The backbuffer is stored bottom row first like a GL framebuffer, so the blit flips it upright
    VkImageBlit blit               = {};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[1]             = {source.width, source.height, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0]             = {0, (int) extent.height, 0};
    blit.dstOffsets[1]             = {(int) extent.width, 0, 1};
    vk.vkCmdBlitImage(frame.cmd, source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageBarrier.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                      &blit, VK_FILTER_NEAREST);
    m_pendingStages |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkRenderPassBeginInfo passInfo = {};
    passInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass            = m_swapchain.GetRenderPass();
    passInfo.framebuffer           = m_swapchain.GetFramebuffer(imageIndex);
    passInfo.renderArea.extent     = extent;
    vk.vkCmdBeginRenderPass(frame.cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (m_imguiPending)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame.cmd);
        m_imguiPending = false;
    }

    vk.vkCmdEndRenderPass(frame.cmd);

    Submit(frame.imageAvailable, m_swapchain.GetRenderFinished(imageIndex));
    VkResult result = m_swapchain.Present(m_context.queue, imageIndex);

    m_frameIndex = (m_frameIndex + 1) % 2;
    BeginFrame();

    if (result != VK_SUCCESS || width != m_swapchainWidth || height != m_swapchainHeight)
    {
        RecreateSwapchain(width, height);
    }
}

void VkRenderDevice::SetVSync(bool enabled)
{
    m_vsync = enabled;
    RecreateSwapchain(m_swapchainWidth, m_swapchainHeight);
}

void VkRenderDevice::GetDrawableSize(SDL_Window *window, int *width, int *height) const
{
    SDL_Vulkan_GetDrawableSize(window, width, height);
}

void VkRenderDevice::BeginFrame()
{
    VkDispatch &vk = m_context.vk;
    Frame &frame   = m_frames[m_frameIndex];
    WaitForSubmit(frame.submit);
    if (!frame.timerNames.empty())
    {
        uint64_t stamps[kMaxGpuTimers * 2];
        if (vk.vkGetQueryPoolResults(m_context.device, frame.timers, 0, (uint32_t) frame.timerNames.size() * 2, sizeof(stamps), stamps,
                                     sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
        {
            uint64_t mask = m_context.timestampValidBits < 64 ? (1ull << m_context.timestampValidBits) - 1 : UINT64_MAX;
            for (size_t i = 0; i < frame.timerNames.size(); i++)
            {
                FrameProfiler::AddGpuTime(frame.timerNames[i],
                                          (double) ((stamps[i * 2 + 1] - stamps[i * 2]) & mask) * m_context.properties.limits.timestampPeriod / 1e6);
            }
        }

        frame.timerNames.clear();
    }

    frame.deletions.Flush(&vk, m_context.device, &m_allocator);
    frame.ring.Reset();
    frame.descriptors.Reset();
    vk.vkResetCommandPool(m_context.device, frame.pool, 0);
    BeginCommands();
    m_frameNumber++;

    frame.timing = FrameProfiler::IsEnabled() && m_context.timestampValidBits > 0;
    if (frame.timing)
    {
        vk.vkCmdResetQueryPool(frame.cmd, frame.timers, 0, kMaxGpuTimers * 2);
    }

    BeginGpuTimer("Gpu.Frame");
    if (m_context.features.rayQuery)
    {
        m_rayScene.Build(frame.cmd, m_frameIndex);
    }
}

void VkRenderDevice::BeginCommands()
{
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    m_context.vk.vkBeginCommandBuffer(m_frames[m_frameIndex].cmd, &info);

    m_dynamicDirty  = true;
    m_boundPipeline = VK_NULL_HANDLE;
    m_uniformsBound = false;
    m_samplersDirty = true;
}

void VkRenderDevice::Submit(VkSemaphore wait, VkSemaphore signal)
{
    VkDispatch &vk = m_context.vk;
    Frame &frame   = m_frames[m_frameIndex];
    vk.vkEndCommandBuffer(frame.cmd);

    // The uploads go in their own batch so they don't wait on the swapchain image
    VkSubmitInfo infos[2] = {};
    uint32_t count        = 0;
    if (frame.uploading)
    {
        VkMemoryBarrier barrier = {};
        barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vk.vkCmdPipelineBarrier(frame.uploadCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0,
                                nullptr);
        vk.vkEndCommandBuffer(frame.uploadCmd);
        infos[0].sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        infos[0].commandBufferCount = 1;
        infos[0].pCommandBuffers    = &frame.uploadCmd;
        frame.uploading             = false;
        count++;
    }

    // The binary semaphore value is ignored, but the array has to line up with the signal semaphores
    VkSemaphore signals[2]                 = {m_timeline, signal};
    uint64_t signalValues[2]               = {m_submitNumber, 0};
    VkTimelineSemaphoreSubmitInfo timeline = {};
    timeline.sType                         = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline.signalSemaphoreValueCount     = signal ? 2 : 1;
    timeline.pSignalSemaphoreValues        = signalValues;

    VkPipelineStageFlags waitStage    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    infos[count].sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    infos[count].pNext                = &timeline;
    infos[count].waitSemaphoreCount   = wait ? 1 : 0;
    infos[count].pWaitSemaphores      = &wait;
    infos[count].pWaitDstStageMask    = &waitStage;
    infos[count].commandBufferCount   = 1;
    infos[count].pCommandBuffers      = &frame.cmd;
    infos[count].signalSemaphoreCount = signal ? 2 : 1;
    infos[count].pSignalSemaphores    = signals;
    count++;
    VkResult result = vk.vkQueueSubmit(m_context.queue, count, infos, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vkQueueSubmit failed with " + std::to_string((int) result));
    }

    frame.submit = m_submitNumber;
    m_submitNumber++;
}

void VkRenderDevice::WaitForSubmit(uint64_t submit)
{
    VkSemaphoreWaitInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    info.semaphoreCount      = 1;
    info.pSemaphores         = &m_timeline;
    info.pValues             = &submit;
    m_context.vk.vkWaitSemaphores(m_context.device, &info, UINT64_MAX);
}

void VkRenderDevice::RecreateSwapchain(int width, int height)
{
    m_context.vk.vkDeviceWaitIdle(m_context.device);
    Logger::LogInfo("Recreating the Vulkan swapchain at %dx%d", width, height);
    m_swapchainWidth  = width;
    m_swapchainHeight = height;
    if (m_swapchain.Create(width, height, m_vsync))
    {
        ResizeBackbuffer(width, height);
    }
}

void VkRenderDevice::ResizeBackbuffer(int width, int height)
{
    VkTextureData &color = m_textures[m_backbufferColor];
    VkTextureData &depth = m_textures[m_backbufferDepth];
    if (color.image && color.width == width && color.height == height)
    {
        return;
    }

    Logger::LogInfo("Resizing the Vulkan backbuffer to %dx%d", width, height);
    ReleaseImage(&color);
    ReleaseImage(&depth);
    CreateImage(&color, width, height, eTextureFormat_RGBA8, 1);
    CreateImage(&depth, width, height, eTextureFormat_Depth32F, 1);
}

void VkRenderDevice::InitImGui(SDL_Window *window)
{
    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_2, LoadImGuiFunction, &m_context);

    ImGui_ImplVulkan_InitInfo info   = {};
    info.ApiVersion                  = VK_API_VERSION_1_2;
    info.Instance                    = m_context.instance;
    info.PhysicalDevice              = m_context.physicalDevice;
    info.Device                      = m_context.device;
    info.QueueFamily                 = m_context.queueFamily;
    info.Queue                       = m_context.queue;
    info.DescriptorPoolSize          = 16;
    info.MinImageCount               = m_swapchain.GetMinImageCount();
    info.ImageCount                  = m_swapchain.GetImageCount();
    info.PipelineInfoMain.RenderPass = m_swapchain.GetRenderPass();
    ImGui_ImplVulkan_Init(&info);
    ImGui_ImplSDL2_InitForVulkan(window);
}

void VkRenderDevice::ShutdownImGui()
{
    m_context.vk.vkDeviceWaitIdle(m_context.device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
}

void VkRenderDevice::BeginImGuiFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void VkRenderDevice::RenderImGui()
{
    m_imguiPending = true;
}

const char *VkRenderDevice::GetVersion() const
{
    return m_version.c_str();
}

const char *VkRenderDevice::GetVendor() const
{
    return m_context.driverProperties.driverName;
}

const char *VkRenderDevice::GetRenderer() const
{
    return m_context.properties.deviceName;
}

VramStats VkRenderDevice::QueryVramStats() const
{
    VramStats stats;
    stats.available  = false;
    stats.hasTotal   = false;
    stats.totalBytes = 0;
    stats.freeBytes  = 0;
    stats.label      = L"";
    if (!m_context.features.memoryBudget)
    {
        return stats;
    }

    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {};
    budget.sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    VkPhysicalDeviceMemoryProperties2 properties     = {};
    properties.sType                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    properties.pNext                                 = &budget;
    m_context.vk.vkGetPhysicalDeviceMemoryProperties2(m_context.physicalDevice, &properties);

    for (uint32_t i = 0; i < properties.memoryProperties.memoryHeapCount; i++)
    {
        if (properties.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            stats.totalBytes += properties.memoryProperties.memoryHeaps[i].size;
            if (budget.heapBudget[i] > budget.heapUsage[i])
            {
                stats.freeBytes += budget.heapBudget[i] - budget.heapUsage[i];
            }
        }
    }

    stats.available = true;
    stats.hasTotal  = stats.totalBytes > 0;
    stats.label     = L"Vulkan";
    return stats;
}

void VkRenderDevice::SetDepthTest(bool enabled)
{
    m_state.depthTest = enabled;
    m_dynamicDirty    = true;
}

void VkRenderDevice::SetDepthMask(bool enabled)
{
    m_state.depthWrite = enabled;
    m_dynamicDirty     = true;
}

void VkRenderDevice::SetDepthFunc(eCOMPARE_FUNC func)
{
    m_state.depthFunc = func;
    m_dynamicDirty    = true;
}

void VkRenderDevice::SetCull(bool enabled)
{
    m_state.cull   = enabled;
    m_dynamicDirty = true;
}

void VkRenderDevice::SetCullFace(eCULL_FACE face)
{
    m_state.cullFace = face;
    m_dynamicDirty   = true;
}

void VkRenderDevice::SetFrontFace(eFRONT_FACE winding)
{
    m_state.frontFace = winding;
    m_dynamicDirty    = true;
}

void VkRenderDevice::SetBlend(bool enabled)
{
    m_state.blend = enabled;
}

void VkRenderDevice::SetBlendFunc(eBLEND_FACTOR src, eBLEND_FACTOR dst)
{
    m_state.srcFactor = src;
    m_state.dstFactor = dst;
}

void VkRenderDevice::SetScissorTest(bool enabled)
{
    m_scissorTest  = enabled;
    m_dynamicDirty = true;
}

void VkRenderDevice::SetScissor(int x, int y, int width, int height)
{
    m_scissor      = {{x, y}, {(uint32_t) width, (uint32_t) height}};
    m_dynamicDirty = true;
}

void VkRenderDevice::SetColorMask(bool r, bool g, bool b, bool a)
{
    m_state.colorMask = (r ? VK_COLOR_COMPONENT_R_BIT : 0) | (g ? VK_COLOR_COMPONENT_G_BIT : 0) | (b ? VK_COLOR_COMPONENT_B_BIT : 0) |
                        (a ? VK_COLOR_COMPONENT_A_BIT : 0);
}

void VkRenderDevice::SetPolygonOffsetFill(bool enabled)
{
    m_state.depthBias = enabled;
    m_dynamicDirty    = true;
}

void VkRenderDevice::SetPolygonOffset(float factor, float units)
{
    m_depthBiasFactor = factor;
    m_depthBiasUnits  = units;
    m_dynamicDirty    = true;
}

void VkRenderDevice::SetPolygonMode(ePOLYGON_MODE mode)
{
    m_state.polygonMode = mode;
}

void VkRenderDevice::SetLineWidth(float width)
{
    m_lineWidth    = width;
    m_dynamicDirty = true;
}

void VkRenderDevice::SetLogicOp(eLOGIC_OP op)
{
    m_state.logicOp = op;
}

void VkRenderDevice::SetViewport(int x, int y, int width, int height)
{
    m_viewport     = {(float) x, (float) y, (float) width, (float) height, 0.0f, 1.0f};
    m_dynamicDirty = true;
}

void VkRenderDevice::SetClearColor(float r, float g, float b, float a)
{
    m_clearColor[0] = r;
    m_clearColor[1] = g;
    m_clearColor[2] = b;
    m_clearColor[3] = a;
}

void VkRenderDevice::SetClearDepth(double depth)
{
    m_clearDepth = (float) depth;
}

void VkRenderDevice::ClearAll()
{
    if (!m_inRenderPass && m_context.features.dynamicRendering && !m_scissorTest)
    {
        BeginRenderPass(m_state.colorMask ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
                        m_state.depthWrite ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD);
        return;
    }

    BeginRenderPass();

    VkClearAttachment attachments[2] = {};
    uint32_t count                   = 0;
    if (m_state.colorMask)
    {
        attachments[count].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        memcpy(attachments[count].clearValue.color.float32, m_clearColor, sizeof(m_clearColor));
        count++;
    }

    if (m_state.depthWrite)
    {
        attachments[count].aspectMask                    = VK_IMAGE_ASPECT_DEPTH_BIT;
        attachments[count].clearValue.depthStencil.depth = m_clearDepth;
        count++;
    }

    if (count == 0)
    {
        return;
    }

    VkClearRect rect = {GetScissorRect(m_textures[m_framebuffers[m_framebuffer].color]), 0, 1};
    m_context.vk.vkCmdClearAttachments(m_frames[m_frameIndex].cmd, count, attachments, 1, &rect);
}

void VkRenderDevice::ClearDepth()
{
    if (!m_state.depthWrite)
    {
        return;
    }

    if (!m_inRenderPass && m_context.features.dynamicRendering && !m_scissorTest)
    {
        BeginRenderPass(VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR);
        return;
    }

    BeginRenderPass();

    VkClearAttachment attachment             = {};
    attachment.aspectMask                    = VK_IMAGE_ASPECT_DEPTH_BIT;
    attachment.clearValue.depthStencil.depth = m_clearDepth;
    VkClearRect rect                         = {GetScissorRect(m_textures[m_framebuffers[m_framebuffer].color]), 0, 1};
    m_context.vk.vkCmdClearAttachments(m_frames[m_frameIndex].cmd, 1, &attachment, 1, &rect);
}

VertexArrayHandle VkRenderDevice::CreateVertexArray()
{
    VertexArrayHandle handle;
    handle.id                 = m_nextId++;
    m_vertexArrays[handle.id] = {};
    MemoryTracker::GetInstance()->GenVertexArrays(1);
    return handle;
}

void VkRenderDevice::DeleteVertexArray(VertexArrayHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    m_vertexArrays.erase(handle.id);
    if (m_vertexArray == handle.id)
    {
        m_vertexArray = 0;
    }

    MemoryTracker::GetInstance()->DeleteVertexArrays(1);
}

void VkRenderDevice::BindVertexArray(VertexArrayHandle handle)
{
    m_vertexArray = handle.id;
}

BufferHandle VkRenderDevice::CreateBuffer()
{
    BufferHandle handle;
    handle.id               = m_nextId++;
    m_buffers[handle.id]    = {};
    m_buffers[handle.id].id = handle.id;
    MemoryTracker::GetInstance()->GenBuffers(1);
    return handle;
}

void VkRenderDevice::DeleteBuffer(BufferHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    ReleaseBuffer(&m_buffers[handle.id]);
    m_buffers.erase(handle.id);
    MemoryTracker::GetInstance()->DeleteBuffers(1);
}

void VkRenderDevice::BindArrayBuffer(BufferHandle handle)
{
    m_arrayBuffer = handle.id;
}

void VkRenderDevice::BindElementArrayBuffer(BufferHandle handle)
{
    m_vertexArrays[m_vertexArray].elementBuffer = handle.id;
}

void VkRenderDevice::UploadArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage)
{
    UploadBuffer(&m_buffers[m_arrayBuffer], data, size, usage);
}

void VkRenderDevice::UploadElementArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage)
{
    UploadBuffer(&m_buffers[m_vertexArrays[m_vertexArray].elementBuffer], data, size, usage);
}

void VkRenderDevice::UpdateArrayBuffer(uint32_t offset, uint32_t size, const void *data)
{
    StageBuffer(&m_buffers[m_arrayBuffer], offset, data, size);
}

void VkRenderDevice::CopyArrayBuffer(BufferHandle source, BufferHandle destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size)
{
    VkBufferData &sourceBuffer      = m_buffers[source.id];
    VkBufferData &destinationBuffer = m_buffers[destination.id];
    VkBufferCopy region             = {sourceOffset, destinationOffset, size};
    m_context.vk.vkCmdCopyBuffer(GetTransferCommands(&destinationBuffer.usage, &sourceBuffer.usage), sourceBuffer.buffer, destinationBuffer.buffer, 1,
                                 &region);
}

bool VkRenderDevice::HasBufferStorage()
{
    return false;
}

void VkRenderDevice::StorageArrayBuffer(uint32_t size)
{
    UploadBuffer(&m_buffers[m_arrayBuffer], nullptr, size, eBufferUsage_Static);
}

void VkRenderDevice::SetVertexAttrib(uint32_t index, int count, eVERTEX_ATTRIB_TYPE type, bool normalized, uint32_t stride, uint32_t offset)
{
    VkVertexArrayData &vertexArray = m_vertexArrays[m_vertexArray];
    vertexArray.attribs[index]     = {m_arrayBuffer, kVertexFormats[type][normalized][count - 1], stride ? stride : count * kAttribTypeSizes[type],
                                      offset, false};
    vertexArray.mask |= 1 << index;
}

void VkRenderDevice::SetVertexAttribDivisor(uint32_t index, uint32_t divisor)
{
    m_vertexArrays[m_vertexArray].attribs[index].instanced = divisor != 0;
}

void VkRenderDevice::UploadBuffer(VkBufferData *buffer, const void *data, uint32_t size, eBUFFER_USAGE usage)
{
    if (usage == eBufferUsage_Stream)
    {
        ReleaseBuffer(buffer);
        uint8_t *mapped = m_frames[m_frameIndex].ring.Allocate(size, &buffer->streamBuffer, &buffer->streamOffset);
        if (data)
        {
            memcpy(mapped, data, size);
        }

        return;
    }

    buffer->streamBuffer = VK_NULL_HANDLE;
    if (buffer->buffer && buffer->size == size)
    {
        if (data)
        {
            StageBuffer(buffer, 0, data, size);
        }

        return;
    }

    ReleaseBuffer(buffer);
    // GL accepts empty buffers but Vulkan does not, so an empty upload leaves the buffer unallocated
    if (size == 0)
    {
        return;
    }

    VkBufferCreateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size               = size;
    info.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
    if (m_context.vk.vkCreateBuffer(m_context.device, &info, nullptr, &buffer->buffer) != VK_SUCCESS)
    {
        Logger::LogError("vkCreateBuffer failed for %u bytes", size);
        throw std::runtime_error("vkCreateBuffer failed");
    }

    m_context.SetObjectName(VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer->buffer, "buffer %u", buffer->id);

    VkMemoryRequirements requirements;
    m_context.vk.vkGetBufferMemoryRequirements(m_context.device, buffer->buffer, &requirements);
    buffer->allocation = m_allocator.Allocate(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
    m_context.vk.vkBindBufferMemory(m_context.device, buffer->buffer, buffer->allocation.memory, buffer->allocation.offset);
    buffer->size  = size;
    buffer->usage = {};
    if (data)
    {
        StageBuffer(buffer, 0, data, size);
    }
}

void VkRenderDevice::ReleaseBuffer(VkBufferData *buffer)
{
    if (buffer->buffer)
    {
        m_frames[m_frameIndex].deletions.PushBuffer(buffer->buffer, buffer->allocation);
        buffer->buffer = VK_NULL_HANDLE;
        buffer->size   = 0;
    }
}

void VkRenderDevice::StageBuffer(VkBufferData *buffer, VkDeviceSize offset, const void *data, VkDeviceSize size)
{
    VkBuffer staging;
    VkDeviceSize stagingOffset;
    memcpy(m_frames[m_frameIndex].ring.Allocate(size, &staging, &stagingOffset), data, size);

    VkBufferCopy region = {stagingOffset, offset, size};
    m_context.vk.vkCmdCopyBuffer(GetTransferCommands(&buffer->usage, nullptr), staging, buffer->buffer, 1, &region);
}

void VkRenderDevice::DrawArrays(ePRIMITIVE_TYPE mode, int first, int count)
{
    VkCommandBuffer cmd = m_frames[m_frameIndex].cmd;
    if (mode != ePrimitiveType_LineLoop)
    {
        PrepareDraw(mode);
        m_context.vk.vkCmdDraw(cmd, count, 1, first, 0);
        return;
    }

    // Vulkan has no line loops, so we close the strip with an index back to the first vertex
    VkBufferData indices = {};
    uint32_t *mapped =
            (uint32_t *) m_frames[m_frameIndex].ring.Allocate((count + 1) * sizeof(uint32_t), &indices.streamBuffer, &indices.streamOffset);
    for (int i = 0; i < count; i++)
    {
        mapped[i] = first + i;
    }

    mapped[count] = first;
    PrepareDraw(mode);
    BindIndexBuffer(indices, 0);
    m_context.vk.vkCmdDrawIndexed(cmd, count + 1, 1, 0, 0, 0);
}

void VkRenderDevice::DrawElements(ePRIMITIVE_TYPE mode, int count, uint32_t offset)
{
    PrepareDraw(mode);
    BindIndexBuffer(m_buffers[m_vertexArrays[m_vertexArray].elementBuffer], offset);
    m_context.vk.vkCmdDrawIndexed(m_frames[m_frameIndex].cmd, count, 1, 0, 0, 0);
}

void VkRenderDevice::DrawMultiElementsIndirect(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, int drawCount)
{
    PrepareDraw(mode);
    BindIndexBuffer(m_buffers[m_vertexArrays[m_vertexArray].elementBuffer], 0);
    VkBufferData &buffer = m_buffers[commands.id];
    buffer.usage.submit  = m_submitNumber;
    m_context.vk.vkCmdDrawIndexedIndirect(m_frames[m_frameIndex].cmd, buffer.buffer ? buffer.buffer : buffer.streamBuffer,
                                          (buffer.buffer ? 0 : buffer.streamOffset) + offset, drawCount, sizeof(DrawIndirectCommand));
}

bool VkRenderDevice::HasDrawIndirectCount()
{
    return m_context.features.drawIndirectCount;
}

void VkRenderDevice::DrawMultiElementsIndirectCount(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, BufferHandle counts,
                                                    uint32_t countOffset, int maxDrawCount)
{
    PrepareDraw(mode);
    BindIndexBuffer(m_buffers[m_vertexArrays[m_vertexArray].elementBuffer], 0);
    m_context.vk.vkCmdDrawIndexedIndirectCount(m_frames[m_frameIndex].cmd, UseBuffer(commands).buffer, offset, UseBuffer(counts).buffer, countOffset,
                                               (uint32_t) maxDrawCount, sizeof(DrawIndirectCommand));
}

bool VkRenderDevice::HasRayQuery()
{
    return m_context.features.rayQuery;
}

void VkRenderDevice::SetRayBoxes(uint32_t geometry, const float *boxes)
{
    m_rayScene.SetBoxes(geometry, boxes);
}

void VkRenderDevice::SetRayInstances(const RayInstance *instances, uint32_t count)
{
    m_rayScene.SetInstances(instances, count);
}

void VkRenderDevice::BindIndexBuffer(VkBufferData &buffer, VkDeviceSize offset)
{
    VkCommandBuffer cmd = m_frames[m_frameIndex].cmd;
    buffer.usage.submit = m_submitNumber;
    if (buffer.buffer)
    {
        m_context.vk.vkCmdBindIndexBuffer(cmd, buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
    }
    else
    {
        m_context.vk.vkCmdBindIndexBuffer(cmd, buffer.streamBuffer, buffer.streamOffset + offset, VK_INDEX_TYPE_UINT32);
    }
}

void VkRenderDevice::PrepareDraw(ePRIMITIVE_TYPE mode)
{
    VkProgramData &program = m_programs[m_program];
    for (uint32_t slot = 0; slot < 16; slot++)
    {
        VkTextureData &texture = m_textures[m_textureBindings[slot]];
        if ((program.samplerMask & (1 << slot)) && texture.layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            // Layouts can't change inside a pass, so a texture we last rendered to or uploaded ends the pass here
            EndRenderPass();
            TransitionImage(m_frames[m_frameIndex].cmd, &texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_READ_BIT_KHR);
        }
    }

    BeginRenderPass();

    VkDispatch &vk                       = m_context.vk;
    Frame &frame                         = m_frames[m_frameIndex];
    const VkFramebufferData &framebuffer = m_framebuffers[m_framebuffer];
    const VkVertexArrayData &vertexArray = m_vertexArrays[m_vertexArray];
    if (m_state.topology != mode)
    {
        m_state.topology = mode;
        m_dynamicDirty   = true;
    }

    // The key is hashed and compared as raw bytes, so it is copied with memcpy to keep the padding zeroed
    VkPipelineKey key;
    memcpy(&key, &m_state, sizeof(key));
    key.program      = m_program;
    key.colorFormat  = kTextureFormats[m_textures[framebuffer.color].format];
    key.instanceMask = 0;
    for (uint32_t location = 0; location < 16; location++)
    {
        if (!(program.inputMask & (1 << location)))
        {
            key.formats[location] = VK_FORMAT_UNDEFINED;
            key.strides[location] = 0;
        }
        else if (vertexArray.mask & (1 << location))
        {
            key.formats[location] = vertexArray.attribs[location].format;
            key.strides[location] = vertexArray.attribs[location].stride;
            if (vertexArray.attribs[location].instanced)
            {
                key.instanceMask |= 1 << location;
            }
        }
        else
        {
            key.formats[location] = VK_FORMAT_R32G32B32A32_SFLOAT;
            key.strides[location] = 0;
        }
    }

    if (m_context.features.extendedDynamicState)
    {
        key.depthTest  = 0;
        key.depthWrite = 0;
        key.depthFunc  = 0;
        key.cull       = 0;
        key.cullFace   = 0;
        key.frontFace  = 0;
        // Dynamic topology still has to stay in the class the pipeline was built with, so lines and triangles keep separate pipelines
        key.topology = mode == ePrimitiveType_Lines || mode == ePrimitiveType_LineLoop ? ePrimitiveType_Lines : ePrimitiveType_Triangles;
    }

    if (m_context.features.extendedDynamicState2)
    {
        key.depthBias = 0;
    }

    VkPipeline pipeline = m_pipelines.Get(key, program);
    if (pipeline != m_boundPipeline)
    {
        vk.vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        m_boundPipeline = pipeline;
    }

    if (m_dynamicDirty)
    {
        VkRect2D scissor = GetScissorRect(m_textures[framebuffer.color]);
        vk.vkCmdSetViewport(frame.cmd, 0, 1, &m_viewport);
        vk.vkCmdSetScissor(frame.cmd, 0, 1, &scissor);
        vk.vkCmdSetDepthBias(frame.cmd, m_depthBiasUnits, 0.0f, m_depthBiasFactor);
        if (m_context.features.wideLines)
        {
            vk.vkCmdSetLineWidth(frame.cmd, m_lineWidth);
        }

        if (m_context.features.extendedDynamicState)
        {
            vk.vkCmdSetCullModeEXT(frame.cmd, m_state.cull ? kCullModes[m_state.cullFace] : VK_CULL_MODE_NONE);
            vk.vkCmdSetFrontFaceEXT(frame.cmd, kFrontFaces[m_state.frontFace]);
            vk.vkCmdSetPrimitiveTopologyEXT(frame.cmd, kTopologies[m_state.topology]);
            vk.vkCmdSetDepthTestEnableEXT(frame.cmd, m_state.depthTest);
            vk.vkCmdSetDepthWriteEnableEXT(frame.cmd, m_state.depthTest && m_state.depthWrite);
            vk.vkCmdSetDepthCompareOpEXT(frame.cmd, kCompareOps[m_state.depthFunc]);
        }

        if (m_context.features.extendedDynamicState2)
        {
            vk.vkCmdSetDepthBiasEnableEXT(frame.cmd, m_state.depthBias);
        }

        m_dynamicDirty = false;
    }

    if (!program.blocks.empty())
    {
        if (program.uniformsDirty || program.uniformFrame != m_frameNumber)
        {
            VkBuffer buffer;
            VkDeviceSize base;
            memcpy(frame.ring.Allocate(program.uniformData.size(), &buffer, &base), program.uniformData.data(), program.uniformData.size());
            if (buffer != program.uniformBuffer || program.uniformFrame != m_frameNumber)
            {
                program.uniformSet = frame.descriptors.Allocate(program.uniformLayout);

                std::vector<VkDescriptorBufferInfo> infos(program.blocks.size());
                std::vector<VkWriteDescriptorSet> writes(program.blocks.size());
                for (size_t i = 0; i < program.blocks.size(); i++)
                {
                    infos[i]                  = {buffer, 0, program.blocks[i].size};
                    writes[i]                 = {};
                    writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writes[i].dstSet          = program.uniformSet;
                    writes[i].dstBinding      = program.blocks[i].binding;
                    writes[i].descriptorCount = 1;
                    writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    writes[i].pBufferInfo     = &infos[i];
                }

                vk.vkUpdateDescriptorSets(m_context.device, (uint32_t) writes.size(), writes.data(), 0, nullptr);
            }

            program.uniformBuffer = buffer;
            program.uniformBase   = base;
            program.uniformFrame  = m_frameNumber;
            program.uniformsDirty = false;
            m_uniformsBound       = false;
        }

        if (!m_uniformsBound)
        {
            uint32_t offsets[16];
            for (size_t i = 0; i < program.bindingOrder.size(); i++)
            {
                offsets[i] = (uint32_t) (program.uniformBase + program.blocks[program.bindingOrder[i]].offset);
            }

            vk.vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program.pipelineLayout, 0, 1, &program.uniformSet,
                                       (uint32_t) program.bindingOrder.size(), offsets);
            m_uniformsBound = true;
        }
    }

    if (m_samplersDirty && program.samplerMask)
    {
        VkDescriptorSet set = VK_NULL_HANDLE;
        if (!m_context.features.pushDescriptor)
        {
            set = frame.descriptors.Allocate(program.samplerLayout);
        }

        VkDescriptorImageInfo infos[16];
        VkWriteDescriptorSet writes[17];
        uint32_t count = 0;
        for (uint32_t slot = 0; slot < 16; slot++)
        {
            if (!(program.samplerMask & (1 << slot)))
            {
                continue;
            }

            VkTextureData &texture        = m_textures[m_textureBindings[slot]];
            texture.usage.submit          = m_submitNumber;
            VkSamplerKey samplerKey       = {texture.minFilter,
                                             texture.magFilter,
                                             texture.wrapS,
                                             texture.wrapT,
                                             (uint16_t) std::min(texture.maxLevel, (int) texture.levels - 1),
                                             texture.anisotropy};
            infos[count]                  = {m_samplers.Get(samplerKey), texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            writes[count]                 = {};
            writes[count].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[count].dstSet          = set;
            writes[count].dstBinding      = slot;
            writes[count].descriptorCount = 1;
            writes[count].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[count].pImageInfo      = &infos[count];
            count++;
        }

        VkAccelerationStructureKHR scene                       = m_rayScene.GetScene();
        VkWriteDescriptorSetAccelerationStructureKHR sceneInfo = {};
        sceneInfo.sType                                        = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        sceneInfo.accelerationStructureCount                   = 1;
        sceneInfo.pAccelerationStructures                      = &scene;
        if (program.sceneBinding >= 0)
        {
            writes[count]                 = {};
            writes[count].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[count].pNext           = &sceneInfo;
            writes[count].dstSet          = set;
            writes[count].dstBinding      = (uint32_t) program.sceneBinding;
            writes[count].descriptorCount = 1;
            writes[count].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            count++;
        }

        if (m_context.features.pushDescriptor)
        {
            vk.vkCmdPushDescriptorSetKHR(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program.pipelineLayout, 1, count, writes);
        }
        else
        {
            vk.vkUpdateDescriptorSets(m_context.device, count, writes, 0, nullptr);
            vk.vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, program.pipelineLayout, 1, 1, &set, 0, nullptr);
        }
        m_samplersDirty = false;
    }

    if (program.inputMask)
    {
        uint32_t bindingCount = 32 - __builtin_clz(program.inputMask);
        VkBuffer buffers[16];
        VkDeviceSize offsets[16];
        for (uint32_t location = 0; location < bindingCount; location++)
        {
            buffers[location] = m_dummyBuffer.buffer;
            offsets[location] = 0;
            if ((program.inputMask & vertexArray.mask) & (1 << location))
            {
                const VkVertexAttrib &attrib = vertexArray.attribs[location];
                VkBufferData &buffer         = m_buffers[attrib.buffer];
                buffer.usage.submit          = m_submitNumber;
                buffers[location]            = buffer.buffer ? buffer.buffer : buffer.streamBuffer;
                offsets[location]            = buffer.buffer ? attrib.offset : buffer.streamOffset + attrib.offset;
            }
        }

        vk.vkCmdBindVertexBuffers(frame.cmd, 0, bindingCount, buffers, offsets);
    }
}

TextureHandle VkRenderDevice::CreateTexture()
{
    TextureHandle handle;
    handle.id = CreateTextureData();
    MemoryTracker::GetInstance()->GenTextures(1);
    return handle;
}

uint32_t VkRenderDevice::CreateTextureData()
{
    uint32_t id            = m_nextId++;
    VkTextureData &texture = m_textures[id];
    texture                = {};
    texture.id             = id;
    texture.minFilter      = eTextureFilter_Linear;
    texture.magFilter      = eTextureFilter_Linear;
    texture.wrapS          = eTextureWrap_Repeat;
    texture.wrapT          = eTextureWrap_Repeat;
    texture.maxLevel       = 1000;
    texture.anisotropy     = 1.0f;
    return id;
}

void VkRenderDevice::DeleteTexture(TextureHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    ReleaseImage(&m_textures[handle.id]);
    m_textures.erase(handle.id);
    for (int i = 0; i < 16; i++)
    {
        if (m_textureBindings[i] == handle.id)
        {
            m_textureBindings[i] = 0;
        }
    }

    MemoryTracker::GetInstance()->DeleteTextures(1);
}

void VkRenderDevice::BindTexture(TextureHandle handle, uint32_t slot)
{
    m_textureBindings[slot] = handle.id;
    m_samplersDirty         = true;
}

void VkRenderDevice::UploadTexture(TextureHandle handle, int level, int width, int height, eTEXTURE_FORMAT format, const void *data)
{
    VkTextureData &texture = m_textures[handle.id];
    if (level == 0)
    {
        if (!texture.image || texture.width != width || texture.height != height || texture.format != format)
        {
            ReleaseImage(&texture);
            CreateImage(&texture, width, height, format, 1);
        }
    }
    else if ((uint32_t) level >= texture.levels)
    {
        // Mip levels arrive one by one after level 0, so we grow to the full chain once and keep the levels already uploaded
        VkTextureData old = texture;
        uint32_t levels   = (uint32_t) std::log2(std::max(texture.width, texture.height)) + 1;
        CreateImage(&texture, texture.width, texture.height, texture.format, levels);

        std::vector<VkImageCopy> regions(old.levels);
        for (uint32_t i = 0; i < old.levels; i++)
        {
            regions[i]                           = {};
            regions[i].srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[i].srcSubresource.mipLevel   = i;
            regions[i].srcSubresource.layerCount = 1;
            regions[i].dstSubresource            = regions[i].srcSubresource;
            regions[i].extent                    = {(uint32_t) std::max(old.width >> i, 1), (uint32_t) std::max(old.height >> i, 1), 1};
        }

        VkCommandBuffer cmd = GetTransferCommands(&texture.usage, &old.usage);
        TransitionImage(cmd, &old, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, VK_ACCESS_2_TRANSFER_READ_BIT_KHR);
        TransitionImage(cmd, &texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
                        VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR);
        m_context.vk.vkCmdCopyImage(cmd, old.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    (uint32_t) regions.size(), regions.data());
        m_frames[m_frameIndex].deletions.PushImage(old.image, old.view, old.allocation);
    }

    if (data)
    {
        if (format == eTextureFormat_RGBA16F)
        {
            throw std::runtime_error("RGBA16F texture uploads are not supported");
        }

        StageImage(&texture, level, 0, 0, width, height, data);
    }
}

void VkRenderDevice::UploadTextureRegion(TextureHandle handle, int x, int y, int width, int height, eTEXTURE_FORMAT format, const void *data)
{
    (void) format;
    StageImage(&m_textures[handle.id], 0, x, y, width, height, data);
}

void VkRenderDevice::SetTextureFilter(TextureHandle handle, eTEXTURE_FILTER minFilter, eTEXTURE_FILTER magFilter)
{
    VkTextureData &texture = m_textures[handle.id];
    texture.minFilter      = minFilter;
    texture.magFilter      = magFilter;
    m_samplersDirty        = true;
}

void VkRenderDevice::SetTextureWrap(TextureHandle handle, eTEXTURE_WRAP wrapS, eTEXTURE_WRAP wrapT)
{
    VkTextureData &texture = m_textures[handle.id];
    texture.wrapS          = wrapS;
    texture.wrapT          = wrapT;
    m_samplersDirty        = true;
}

void VkRenderDevice::SetTextureMaxLevel(TextureHandle handle, int maxLevel)
{
    m_textures[handle.id].maxLevel = maxLevel;
    m_samplersDirty                = true;
}

void VkRenderDevice::SetTextureMaxAnisotropy(TextureHandle handle, float anisotropy)
{
    m_textures[handle.id].anisotropy = anisotropy;
    m_samplersDirty                  = true;
}

float VkRenderDevice::GetMaxTextureAnisotropy() const
{
    return m_context.features.samplerAnisotropy ? m_context.properties.limits.maxSamplerAnisotropy : 1.0f;
}

void VkRenderDevice::CreateImage(VkTextureData *texture, int width, int height, eTEXTURE_FORMAT format, uint32_t levels)
{
    VkDispatch &vk = m_context.vk;
    bool depth     = format == eTextureFormat_Depth32F;

    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType         = VK_IMAGE_TYPE_2D;
    info.format            = kTextureFormats[format];
    info.extent            = {(uint32_t) width, (uint32_t) height, 1};
    info.mipLevels         = levels;
    info.arrayLayers       = 1;
    info.samples           = VK_SAMPLE_COUNT_1_BIT;
    info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    info.usage             = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                             (depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vk.vkCreateImage(m_context.device, &info, nullptr, &texture->image) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateImage failed");
    }

    m_context.SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t) texture->image, "texture %u", texture->id);

    VkMemoryRequirements requirements;
    vk.vkGetImageMemoryRequirements(m_context.device, texture->image, &requirements);
    texture->allocation = m_allocator.Allocate(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, false);
    vk.vkBindImageMemory(m_context.device, texture->image, texture->allocation.memory, texture->allocation.offset);

    VkImageSubresourceRange range = {(VkImageAspectFlags) (depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT), 0, levels, 0, 1};

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                 = texture->image;
    viewInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                = info.format;
    viewInfo.subresourceRange      = range;
    if (vk.vkCreateImageView(m_context.device, &viewInfo, nullptr, &texture->view) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateImageView failed");
    }

    m_context.SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) texture->view, "texture %u view", texture->id);

    texture->format = format;
    texture->width  = width;
    texture->height = height;
    texture->levels = levels;
    texture->usage  = {};
    texture->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    texture->stages = 0;
    texture->access = 0;
    texture->generation++;
    m_samplersDirty = true;
}

void VkRenderDevice::ReleaseImage(VkTextureData *texture)
{
    if (texture->image)
    {
        m_frames[m_frameIndex].deletions.PushImage(texture->image, texture->view, texture->allocation);
        texture->image = VK_NULL_HANDLE;
        texture->view  = VK_NULL_HANDLE;
    }
}

void VkRenderDevice::StageImage(VkTextureData *texture, int level, int x, int y, int width, int height, const void *data)
{
    VkDeviceSize size = (VkDeviceSize) width * height * kBytesPerPixel[texture->format];
    VkBuffer staging;
    VkDeviceSize stagingOffset;
    memcpy(m_frames[m_frameIndex].ring.Allocate(size, &staging, &stagingOffset), data, size);

    VkBufferImageCopy region           = {};
    region.bufferOffset                = stagingOffset;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel   = level;
    region.imageSubresource.layerCount = 1;
    region.imageOffset                 = {x, y, 0};
    region.imageExtent                 = {(uint32_t) width, (uint32_t) height, 1};
    VkCommandBuffer cmd                = GetTransferCommands(&texture->usage, nullptr);
    TransitionImage(cmd, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR);
    m_context.vk.vkCmdCopyBufferToImage(cmd, staging, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

ProgramHandle VkRenderDevice::CreateProgram(const char *vertexPath, const char *fragmentPath, const std::vector<ShaderUniformBlock> &blocks,
                                            const std::vector<ShaderSampler> &samplers)
{
    VkDispatch &vk  = m_context.vk;
    VkDevice device = m_context.device;

    VkProgramData program = {};
    program.name          = std::string(vertexPath) + "|" + fragmentPath;
    program.sceneBinding  = -1;
    program.blocks.resize(blocks.size());
    const char *paths[2]       = {vertexPath, fragmentPath};
    VkShaderModule *modules[2] = {&program.vertex, &program.fragment};
    for (int stage = 0; stage < 2; stage++)
    {
        std::vector<uint8_t> code           = File(std::string(paths[stage]) + ".spv").ReadAllBytes();
        VkShaderModuleCreateInfo moduleInfo = {};
        moduleInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.codeSize                 = code.size();
        moduleInfo.pCode                    = (const uint32_t *) code.data();
        if (vk.vkCreateShaderModule(device, &moduleInfo, nullptr, modules[stage]) != VK_SUCCESS)
        {
            throw std::runtime_error(std::string("vkCreateShaderModule failed for ") + paths[stage]);
        }

        nlohmann::json reflection = nlohmann::json::parse(File(std::string(paths[stage]) + ".json").ReadAllText());
        if (reflection.contains("ubos"))
        {
            for (const nlohmann::json &ubo : reflection["ubos"])
            {
                std::string name = ubo["name"].get<std::string>();
                for (size_t i = 0; i < blocks.size(); i++)
                {
                    if (blocks[i].name == name)
                    {
                        program.blocks[i].binding = ubo["binding"].get<uint32_t>();
                    }
                }
            }
        }

        if (reflection.contains("acceleration_structures"))
        {
            program.sceneBinding = reflection["acceleration_structures"][0]["binding"].get<int>();
        }

        if (stage == 0 && reflection.contains("inputs"))
        {
            for (const nlohmann::json &input : reflection["inputs"])
            {
                program.inputMask |= 1 << input["location"].get<uint32_t>();
            }
        }
    }

    VkDeviceSize alignment = m_context.properties.limits.minUniformBufferOffsetAlignment;
    uint32_t offset        = 0;
    std::vector<VkDescriptorSetLayoutBinding> uniformBindings(blocks.size());
    for (size_t i = 0; i < blocks.size(); i++)
    {
        program.blocks[i].offset = offset;
        program.blocks[i].size   = blocks[i].size;
        offset                   = (uint32_t) ((offset + blocks[i].size + alignment - 1) / alignment * alignment);
        uniformBindings[i]       = {program.blocks[i].binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
                                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        program.bindingOrder.push_back((uint32_t) i);
    }

    std::sort(program.bindingOrder.begin(), program.bindingOrder.end(),
              [&program](uint32_t a, uint32_t b)
              {
                  return program.blocks[a].binding < program.blocks[b].binding;
              });
    program.uniformData.resize(offset);

    std::vector<VkDescriptorSetLayoutBinding> samplerBindings(samplers.size());
    for (size_t i = 0; i < samplers.size(); i++)
    {
        program.samplerMask |= 1 << samplers[i].binding;
        samplerBindings[i] = {samplers[i].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    }

    if (program.sceneBinding >= 0)
    {
        samplerBindings.push_back(
                {(uint32_t) program.sceneBinding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount                    = (uint32_t) uniformBindings.size();
    layoutInfo.pBindings                       = uniformBindings.data();
    if (vk.vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &program.uniformLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateDescriptorSetLayout failed");
    }

    layoutInfo.bindingCount = (uint32_t) samplerBindings.size();
    layoutInfo.pBindings    = samplerBindings.data();
    if (m_context.features.pushDescriptor)
    {
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    }

    if (vk.vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &program.samplerLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateDescriptorSetLayout failed");
    }

    VkDescriptorSetLayout setLayouts[2]       = {program.uniformLayout, program.samplerLayout};
    VkPipelineLayoutCreateInfo pipelineLayout = {};
    pipelineLayout.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.setLayoutCount             = 2;
    pipelineLayout.pSetLayouts                = setLayouts;
    if (vk.vkCreatePipelineLayout(device, &pipelineLayout, nullptr, &program.pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreatePipelineLayout failed");
    }

    program.uniformsDirty = true;

    ProgramHandle handle;
    handle.id = m_nextId++;
    m_pipelines.Prewarm(handle.id, program);
    m_programs[handle.id] = std::move(program);
    return handle;
}

void VkRenderDevice::DeleteProgram(ProgramHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    VkDeletionQueue &deletions = m_frames[m_frameIndex].deletions;
    if (m_computePrograms.count(handle.id))
    {
        const VkComputeProgramData &compute = m_computePrograms[handle.id];
        deletions.PushPipeline(compute.pipeline);
        deletions.PushProgram(compute.module, VK_NULL_HANDLE, compute.layout, VK_NULL_HANDLE, compute.pipelineLayout);
        m_computePrograms.erase(handle.id);
        return;
    }

    const VkProgramData &program = m_programs[handle.id];
    m_pipelines.RemoveProgram(handle.id, &deletions);
    deletions.PushProgram(program.vertex, program.fragment, program.uniformLayout, program.samplerLayout, program.pipelineLayout);
    m_programs.erase(handle.id);
}

void VkRenderDevice::UseProgram(ProgramHandle handle)
{
    if (handle.id != m_program)
    {
        m_program       = handle.id;
        m_uniformsBound = false;
        m_samplersDirty = true;
    }
}

void VkRenderDevice::UploadUniformBlock(ProgramHandle handle, int block, int firstVec4, int vec4Count, const float *data)
{
    VkProgramData &program = m_programs[handle.id];
    memcpy(program.uniformData.data() + program.blocks[block].offset + firstVec4 * 16, data, vec4Count * 16);
    program.uniformsDirty = true;
}

ProgramHandle VkRenderDevice::CreateComputeProgram(const char *path, const ShaderUniformBlock &block, int bufferCount,
                                                   const std::vector<ShaderSampler> &samplers)
{
    VkDispatch &vk  = m_context.vk;
    VkDevice device = m_context.device;

    VkComputeProgramData program        = {};
    program.uniformSize                 = block.size;
    std::vector<uint8_t> code           = File(std::string(path) + ".spv").ReadAllBytes();
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize                 = code.size();
    moduleInfo.pCode                    = (const uint32_t *) code.data();
    if (vk.vkCreateShaderModule(device, &moduleInfo, nullptr, &program.module) != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("vkCreateShaderModule failed for ") + path);
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings(bufferCount + 1);
    bindings[0] = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    for (int i = 1; i <= bufferCount; i++)
    {
        bindings[i] = {(uint32_t) i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    }

    for (size_t i = 0; i < samplers.size(); i++)
    {
        bindings.push_back({samplers[i].binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
        program.samplerBindings.push_back(samplers[i].binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount                    = (uint32_t) bindings.size();
    layoutInfo.pBindings                       = bindings.data();
    if (vk.vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &program.layout) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateDescriptorSetLayout failed");
    }

    VkPipelineLayoutCreateInfo pipelineLayout = {};
    pipelineLayout.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.setLayoutCount             = 1;
    pipelineLayout.pSetLayouts                = &program.layout;
    if (vk.vkCreatePipelineLayout(device, &pipelineLayout, nullptr, &program.pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreatePipelineLayout failed");
    }

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage                 = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module                = program.module;
    pipelineInfo.stage.pName                 = "main";
    pipelineInfo.layout                      = program.pipelineLayout;
    if (vk.vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &program.pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("vkCreateComputePipelines failed for ") + path);
    }

    ProgramHandle handle;
    handle.id                    = m_nextId++;
    m_computePrograms[handle.id] = program;
    return handle;
}

void VkRenderDevice::ClearBuffer(BufferHandle handle)
{
    EndRenderPass();
    m_pendingStages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    MainBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    m_context.vk.vkCmdFillBuffer(m_frames[m_frameIndex].cmd, UseBuffer(handle).buffer, 0, VK_WHOLE_SIZE, 0);
    m_pendingStages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    m_pendingAccess |= VK_ACCESS_TRANSFER_WRITE_BIT;
}

void VkRenderDevice::DispatchCompute(ProgramHandle handle, const float *uniforms, const BufferHandle *buffers, int bufferCount,
                                     const TextureHandle *textures, uint32_t groupCount)
{
    VkDispatch &vk                      = m_context.vk;
    Frame &frame                        = m_frames[m_frameIndex];
    const VkComputeProgramData &program = m_computePrograms[handle.id];

    // Draws from earlier submits may still read the buffers we write, so we always wait on indirect reads
    EndRenderPass();
    for (size_t i = 0; i < program.samplerBindings.size(); i++)
    {
        TransitionImage(frame.cmd, &m_textures[textures[i].id], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR,
                        VK_ACCESS_2_SHADER_READ_BIT_KHR);
    }

    m_pendingStages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    MainBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

    VkBuffer uniformBuffer;
    VkDeviceSize uniformOffset;
    memcpy(frame.ring.Allocate(program.uniformSize, &uniformBuffer, &uniformOffset), uniforms, program.uniformSize);

    VkDescriptorSet set = frame.descriptors.Allocate(program.layout);
    std::vector<VkDescriptorBufferInfo> infos(bufferCount + 1);
    std::vector<VkWriteDescriptorSet> writes(bufferCount + 1);
    infos[0] = {uniformBuffer, uniformOffset, program.uniformSize};
    for (int i = 1; i <= bufferCount; i++)
    {
        VkBufferData &buffer = UseBuffer(buffers[i - 1]);
        infos[i]             = {buffer.buffer, 0, buffer.size};
    }

    for (int i = 0; i <= bufferCount; i++)
    {
        writes[i]                 = {};
        writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet          = set;
        writes[i].dstBinding      = (uint32_t) i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType  = i == 0 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo     = &infos[i];
    }

    std::vector<VkDescriptorImageInfo> imageInfos(program.samplerBindings.size());
    for (size_t i = 0; i < program.samplerBindings.size(); i++)
    {
        VkTextureData &texture  = m_textures[textures[i].id];
        texture.usage.submit    = m_submitNumber;
        VkSamplerKey samplerKey = {
                texture.minFilter, texture.magFilter, texture.wrapS, texture.wrapT, (uint16_t) std::min(texture.maxLevel, (int) texture.levels - 1),
                texture.anisotropy};
        imageInfos[i] = {m_samplers.Get(samplerKey), texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        VkWriteDescriptorSet &write = writes.emplace_back();
        write                       = {};
        write.sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet                = set;
        write.dstBinding            = program.samplerBindings[i];
        write.descriptorCount       = 1;
        write.descriptorType        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo            = &imageInfos[i];
    }

    vk.vkUpdateDescriptorSets(m_context.device, (uint32_t) writes.size(), writes.data(), 0, nullptr);
    vk.vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, program.pipeline);
    vk.vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, program.pipelineLayout, 0, 1, &set, 0, nullptr);
    vk.vkCmdDispatch(frame.cmd, groupCount, 1, 1);
    m_pendingStages |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    m_pendingAccess |= VK_ACCESS_SHADER_WRITE_BIT;
}

FramebufferHandle VkRenderDevice::CreateFramebuffer(TextureHandle color, TextureHandle depth)
{
    FramebufferHandle handle;
    handle.id                 = m_nextId++;
    m_framebuffers[handle.id] = {color.id, depth.id, 0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE};
    Logger::LogInfo("Created Vulkan framebuffer %u with color %u and depth %u", handle.id, color.id, depth.id);
    return handle;
}

void VkRenderDevice::DeleteFramebuffer(FramebufferHandle handle)
{
    if (!handle.IsValid())
    {
        return;
    }

    if (m_framebuffer == handle.id)
    {
        EndRenderPass();
        m_framebuffer = 0;
    }

    VkFramebufferData &framebuffer = m_framebuffers[handle.id];
    if (framebuffer.framebuffer)
    {
        m_frames[m_frameIndex].deletions.PushFramebuffer(framebuffer.framebuffer);
    }

    m_framebuffers.erase(handle.id);
}

void VkRenderDevice::BindFramebuffer(FramebufferHandle handle)
{
    if (handle.id != m_framebuffer)
    {
        EndRenderPass();
        m_framebuffer = handle.id;
    }
}

void VkRenderDevice::BlitFramebufferDepth(FramebufferHandle source, int sourceWidth, int sourceHeight, FramebufferHandle destination,
                                          int destinationWidth, int destinationHeight)
{
    VkTextureData &sourceDepth           = m_textures[m_framebuffers[source.id].depth];
    VkTextureData &destinationDepth      = m_textures[m_framebuffers[destination.id].depth];
    VkCommandBuffer cmd                  = GetTransferCommands(&destinationDepth.usage, &sourceDepth.usage);
    VkImage sourceImage                  = sourceDepth.image;
    VkImage destinationImage             = destinationDepth.image;
    VkImageSubresourceLayers subresource = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1};
    TransitionImage(cmd, &sourceDepth, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, VK_ACCESS_2_TRANSFER_READ_BIT_KHR);
    TransitionImage(cmd, &destinationDepth, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR);
    if (sourceWidth == destinationWidth && sourceHeight == destinationHeight)
    {
        VkImageCopy region    = {};
        region.srcSubresource = subresource;
        region.dstSubresource = subresource;
        region.extent         = {(uint32_t) sourceWidth, (uint32_t) sourceHeight, 1};
        m_context.vk.vkCmdCopyImage(cmd, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                    &region);
    }
    else
    {
        VkImageBlit blit    = {};
        blit.srcSubresource = subresource;
        blit.srcOffsets[1]  = {sourceWidth, sourceHeight, 1};
        blit.dstSubresource = subresource;
        blit.dstOffsets[1]  = {destinationWidth, destinationHeight, 1};
        m_context.vk.vkCmdBlitImage(cmd, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                    &blit, VK_FILTER_NEAREST);
    }

    BindFramebuffer(destination);
}

void VkRenderDevice::ReadPixels(int x, int y, int width, int height, uint8_t *pixels)
{
    Logger::LogWarn("ReadPixels %dx%d waits for the GPU to go idle", width, height);
    VkDispatch &vk = m_context.vk;
    Frame &frame   = m_frames[m_frameIndex];
    EndRenderPass();
    MainBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    m_pendingStages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkDeviceSize size = (VkDeviceSize) width * height * 4;
    VkBuffer staging;
    VkDeviceSize stagingOffset;
    uint8_t *mapped = frame.ring.Allocate(size, &staging, &stagingOffset);

    VkBufferImageCopy region           = {};
    region.bufferOffset                = stagingOffset;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset                 = {x, y, 0};
    region.imageExtent                 = {(uint32_t) width, (uint32_t) height, 1};
    VkTextureData &color               = m_textures[m_framebuffers[m_framebuffer].color];
    TransitionImage(frame.cmd, &color, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, VK_ACCESS_2_TRANSFER_READ_BIT_KHR);
    vk.vkCmdCopyImageToBuffer(frame.cmd, color.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, 1, &region);

    VkMemoryBarrier barrier = {};
    barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask   = VK_ACCESS_HOST_READ_BIT;
    vk.vkCmdPipelineBarrier(frame.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    // We keep the ring and descriptors alive because earlier draws in this frame were already recorded against them
    Submit(VK_NULL_HANDLE, VK_NULL_HANDLE);
    WaitForSubmit(frame.submit);
    vk.vkResetCommandPool(m_context.device, frame.pool, 0);
    BeginCommands();
    memcpy(pixels, mapped, size);
}

void VkRenderDevice::BeginTextureReadback(TextureHandle handle, int width, int height, eTEXTURE_FORMAT format)
{
    VkDispatch &vk         = m_context.vk;
    Frame &frame           = m_frames[m_frameIndex];
    m_readbackTarget       = 1 - m_readbackTarget;
    VkBufferData &readback = m_readbacks[m_readbackTarget];
    VkDeviceSize size      = (VkDeviceSize) width * height * kBytesPerPixel[format];
    if (readback.size != size)
    {
        ReleaseBuffer(&readback);
        VkBufferCreateInfo info = {};
        info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size               = size;
        info.usage              = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        if (vk.vkCreateBuffer(m_context.device, &info, nullptr, &readback.buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateBuffer failed for a texture readback");
        }

        VkMemoryRequirements requirements;
        vk.vkGetBufferMemoryRequirements(m_context.device, readback.buffer, &requirements);
        // Uncached host memory reads at a crawl from the CPU, so readbacks need the cached kind
        readback.allocation = m_allocator.Allocate(
                requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, true);
        vk.vkBindBufferMemory(m_context.device, readback.buffer, readback.allocation.memory, readback.allocation.offset);
        readback.size = size;
    }

    EndRenderPass();
    VkTextureData &texture             = m_textures[handle.id];
    VkBufferImageCopy region           = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent                 = {(uint32_t) width, (uint32_t) height, 1};
    TransitionImage(frame.cmd, &texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
                    VK_ACCESS_2_TRANSFER_READ_BIT_KHR);
    vk.vkCmdCopyImageToBuffer(frame.cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &region);

    VkMemoryBarrier barrier = {};
    barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask   = VK_ACCESS_HOST_READ_BIT;
    vk.vkCmdPipelineBarrier(frame.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    m_readbackSubmit  = m_submitNumber;
    m_readbackPending = true;
}

const uint8_t *VkRenderDevice::PollTextureReadback()
{
    if (!m_readbackPending)
    {
        return nullptr;
    }

    uint64_t completed;
    m_context.vk.vkGetSemaphoreCounterValue(m_context.device, m_timeline, &completed);
    if (completed < m_readbackSubmit)
    {
        return nullptr;
    }

    m_readbackPending = false;
    return m_readbacks[m_readbackTarget].allocation.mapped;
}

void VkRenderDevice::BeginGpuTimer(const char *name)
{
    Frame &frame = m_frames[m_frameIndex];
    if (!frame.timing || frame.timerNames.size() == kMaxGpuTimers)
    {
        m_openTimers.push_back(UINT32_MAX);
        return;
    }

    uint32_t index = (uint32_t) frame.timerNames.size();
    frame.timerNames.push_back(name);
    m_openTimers.push_back(index);
    m_context.vk.vkCmdWriteTimestamp(frame.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.timers, index * 2);
}

void VkRenderDevice::EndGpuTimer()
{
    uint32_t index = m_openTimers.back();
    m_openTimers.pop_back();
    if (index != UINT32_MAX)
    {
        m_context.vk.vkCmdWriteTimestamp(m_frames[m_frameIndex].cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_frames[m_frameIndex].timers,
                                         index * 2 + 1);
    }
}

void VkRenderDevice::BeginRenderPass(VkAttachmentLoadOp colorLoadOp, VkAttachmentLoadOp depthLoadOp)
{
    if (m_inRenderPass)
    {
        return;
    }

    VkDispatch &vk = m_context.vk;
    Frame &frame   = m_frames[m_frameIndex];

    MainBarrier(kGraphicsStages, kGraphicsAccess);

    const VkFramebufferData &attachments = m_framebuffers[m_framebuffer];
    TransitionImage(frame.cmd, &m_textures[attachments.color], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                    VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR);
    TransitionImage(frame.cmd, &m_textures[attachments.depth], VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR,
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR);

    if (m_context.features.dynamicRendering)
    {
        const VkFramebufferData &framebuffer = m_framebuffers[m_framebuffer];
        VkTextureData &color                 = m_textures[framebuffer.color];
        VkTextureData &depth                 = m_textures[framebuffer.depth];
        color.usage.submit                   = m_submitNumber;
        depth.usage.submit                   = m_submitNumber;

        VkRenderingAttachmentInfoKHR colorAttachment = {};
        colorAttachment.sType                        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageView                    = color.view;
        colorAttachment.imageLayout                  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp                       = colorLoadOp;
        colorAttachment.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
        memcpy(colorAttachment.clearValue.color.float32, m_clearColor, sizeof(m_clearColor));

        VkRenderingAttachmentInfoKHR depthAttachment  = {};
        depthAttachment.sType                         = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        depthAttachment.imageView                     = depth.view;
        depthAttachment.imageLayout                   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp                        = depthLoadOp;
        depthAttachment.storeOp                       = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil.depth = m_clearDepth;

        VkRenderingInfoKHR info   = {};
        info.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        info.renderArea.extent    = {(uint32_t) color.width, (uint32_t) color.height};
        info.layerCount           = 1;
        info.colorAttachmentCount = 1;
        info.pColorAttachments    = &colorAttachment;
        info.pDepthAttachment     = &depthAttachment;
        vk.vkCmdBeginRenderingKHR(frame.cmd, &info);
    }
    else
    {
        const VkFramebufferData &framebuffer       = GetFramebuffer();
        VkTextureData &color                       = m_textures[framebuffer.color];
        color.usage.submit                         = m_submitNumber;
        m_textures[framebuffer.depth].usage.submit = m_submitNumber;

        VkRenderPassBeginInfo info = {};
        info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass            = framebuffer.renderPass;
        info.framebuffer           = framebuffer.framebuffer;
        info.renderArea.extent     = {(uint32_t) color.width, (uint32_t) color.height};
        vk.vkCmdBeginRenderPass(frame.cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    m_inRenderPass  = true;
    m_dynamicDirty  = true;
    m_boundPipeline = VK_NULL_HANDLE;
    m_uniformsBound = false;
    m_samplersDirty = true;
}

void VkRenderDevice::EndRenderPass()
{
    if (m_inRenderPass)
    {
        if (m_context.features.dynamicRendering)
        {
            m_context.vk.vkCmdEndRenderingKHR(m_frames[m_frameIndex].cmd);
        }
        else
        {
            m_context.vk.vkCmdEndRenderPass(m_frames[m_frameIndex].cmd);
        }
        m_inRenderPass = false;
        m_pendingStages |= kGraphicsStages;
    }
}

void VkRenderDevice::MainBarrier(VkPipelineStageFlags dstStages, VkAccessFlags dstAccess)
{
    if (!m_pendingStages)
    {
        return;
    }

    VkMemoryBarrier barrier = {};
    barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask   = m_pendingAccess;
    barrier.dstAccessMask   = dstAccess;
    m_context.vk.vkCmdPipelineBarrier(m_frames[m_frameIndex].cmd, m_pendingStages, dstStages, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    m_pendingStages = 0;
    m_pendingAccess = 0;
}

void VkRenderDevice::TransitionImage(VkCommandBuffer cmd, VkTextureData *texture, VkImageLayout layout, VkPipelineStageFlags2KHR stages,
                                     VkAccessFlags2KHR access)
{
    constexpr VkAccessFlags2KHR kWriteAccess =
            VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
    if (texture->layout == layout && !((texture->access | access) & kWriteAccess))
    {
        texture->stages |= stages;
        texture->access |= access;
        return;
    }

    VkImageAspectFlags aspect = texture->format == eTextureFormat_Depth32F ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    if (m_context.features.synchronization2)
    {
        VkImageMemoryBarrier2KHR barrier = {};
        barrier.sType                    = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        barrier.srcStageMask             = texture->stages;
        barrier.srcAccessMask            = texture->access & kWriteAccess;
        barrier.dstStageMask             = stages;
        barrier.dstAccessMask            = access;
        barrier.oldLayout                = texture->layout;
        barrier.newLayout                = layout;
        barrier.srcQueueFamilyIndex      = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex      = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                    = texture->image;
        barrier.subresourceRange         = {aspect, 0, texture->levels, 0, 1};

        VkDependencyInfoKHR dependency     = {};
        dependency.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers    = &barrier;
        m_context.vk.vkCmdPipelineBarrier2KHR(cmd, &dependency);
    }
    else
    {
        // Every sync2 bit we pass here has the same value in the legacy flags
        VkImageMemoryBarrier barrier = {};
        barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask        = (VkAccessFlags) (texture->access & kWriteAccess);
        barrier.dstAccessMask        = (VkAccessFlags) access;
        barrier.oldLayout            = texture->layout;
        barrier.newLayout            = layout;
        barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                = texture->image;
        barrier.subresourceRange     = {aspect, 0, texture->levels, 0, 1};
        m_context.vk.vkCmdPipelineBarrier(cmd, (VkPipelineStageFlags) (texture->stages ? texture->stages : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR),
                                          (VkPipelineStageFlags) stages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    texture->layout = layout;
    texture->stages = stages;
    texture->access = access;
}

void VkRenderDevice::BeginUploads()
{
    Frame &frame = m_frames[m_frameIndex];
    if (frame.uploading)
    {
        return;
    }

    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    m_context.vk.vkBeginCommandBuffer(frame.uploadCmd, &info);

    VkMemoryBarrier barrier = {};
    barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask   = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask   = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    m_context.vk.vkCmdPipelineBarrier(frame.uploadCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr,
                                      0, nullptr);
    frame.uploading = true;
    m_uploadEpoch++;
}

// We hoist transfers on resources this submit hasn't drawn with into the upload buffer so they don't split render passes
VkCommandBuffer VkRenderDevice::GetTransferCommands(VkUsage *destination, VkUsage *source)
{
    Frame &frame = m_frames[m_frameIndex];
    if (destination->submit == m_submitNumber || (source && source->submit == m_submitNumber))
    {
        EndRenderPass();
        MainBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
        m_pendingStages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        m_pendingAccess |= VK_ACCESS_TRANSFER_WRITE_BIT;
        destination->submit = m_submitNumber;
        if (source)
        {
            source->submit = m_submitNumber;
        }

        return frame.cmd;
    }

    BeginUploads();
    if (destination->uploadEpoch == m_uploadEpoch || (source && source->uploadEpoch == m_uploadEpoch))
    {
        VkMemoryBarrier barrier = {};
        barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask   = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        m_context.vk.vkCmdPipelineBarrier(frame.uploadCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr,
                                          0, nullptr);
        m_uploadEpoch++;
    }

    destination->uploadEpoch = m_uploadEpoch;
    if (source)
    {
        source->uploadEpoch = m_uploadEpoch;
    }

    return frame.uploadCmd;
}

VkFramebufferData &VkRenderDevice::GetFramebuffer()
{
    VkFramebufferData &framebuffer = m_framebuffers[m_framebuffer];
    const VkTextureData &color     = m_textures[framebuffer.color];
    const VkTextureData &depth     = m_textures[framebuffer.depth];
    if (framebuffer.framebuffer && framebuffer.colorGeneration == color.generation && framebuffer.depthGeneration == depth.generation)
    {
        return framebuffer;
    }

    if (framebuffer.framebuffer)
    {
        m_frames[m_frameIndex].deletions.PushFramebuffer(framebuffer.framebuffer);
    }

    framebuffer.renderPass = m_pipelines.GetRenderPass(kTextureFormats[color.format]);

    VkImageView views[2]         = {color.view, depth.view};
    VkFramebufferCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass              = framebuffer.renderPass;
    info.attachmentCount         = 2;
    info.pAttachments            = views;
    info.width                   = color.width;
    info.height                  = color.height;
    info.layers                  = 1;
    if (m_context.vk.vkCreateFramebuffer(m_context.device, &info, nullptr, &framebuffer.framebuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateFramebuffer failed");
    }

    framebuffer.colorGeneration = color.generation;
    framebuffer.depthGeneration = depth.generation;
    return framebuffer;
}

VkRect2D VkRenderDevice::GetScissorRect(const VkTextureData &color) const
{
    if (!m_scissorTest)
    {
        return {{0, 0}, {(uint32_t) color.width, (uint32_t) color.height}};
    }

    int x0 = std::clamp(m_scissor.offset.x, 0, color.width);
    int y0 = std::clamp(m_scissor.offset.y, 0, color.height);
    int x1 = std::clamp(m_scissor.offset.x + (int) m_scissor.extent.width, x0, color.width);
    int y1 = std::clamp(m_scissor.offset.y + (int) m_scissor.extent.height, y0, color.height);
    return {{x0, y0}, {(uint32_t) (x1 - x0), (uint32_t) (y1 - y0)}};
}

#endif
