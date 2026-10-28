#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkSwapchain.h"

#include <algorithm>
#include <stdexcept>

#include "../../core/Logger.h"

VkSwapchain::VkSwapchain()
{
    m_vk             = nullptr;
    m_physicalDevice = VK_NULL_HANDLE;
    m_device         = VK_NULL_HANDLE;
    m_surface        = VK_NULL_HANDLE;
    m_format         = {};
    m_renderPass     = VK_NULL_HANDLE;
    m_swapchain      = VK_NULL_HANDLE;
    m_extent         = {};
    m_minImageCount  = 0;
}

void VkSwapchain::Init(VkDispatch *vk, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface)
{
    m_vk             = vk;
    m_physicalDevice = physicalDevice;
    m_device         = device;
    m_surface        = surface;

    uint32_t formatCount = 0;
    m_vk->vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    m_vk->vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    m_format = formats[0];
    for (size_t i = 0; i < formats.size(); i++)
    {
        if ((formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            m_format = formats[i];
            break;
        }
    }

    CreateRenderPass();
}

void VkSwapchain::CreateRenderPass()
{
    VkAttachmentDescription attachment = {};
    attachment.format                  = m_format.format;
    attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference reference = {};
    reference.attachment            = 0;
    reference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &reference;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount        = 1;
    info.pAttachments           = &attachment;
    info.subpassCount           = 1;
    info.pSubpasses             = &subpass;
    info.dependencyCount        = 1;
    info.pDependencies          = &dependency;

    if (m_vk->vkCreateRenderPass(m_device, &info, nullptr, &m_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateRenderPass failed for the swapchain");
    }
}

bool VkSwapchain::Create(uint32_t width, uint32_t height, bool vsync)
{
    VkSurfaceCapabilitiesKHR capabilities;
    m_vk->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

    uint32_t modeCount = 0;
    m_vk->vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    m_vk->vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &modeCount, modes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!vsync)
    {
        // Mailbox still waits for the next refresh to show a frame, so we prefer immediate for the lowest mouse latency
        if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR) != modes.end())
        {
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        else if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end())
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        m_extent = capabilities.currentExtent;
    }
    else
    {
        m_extent.width  = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // Windows reports a zero extent while the window is minimized and a swapchain can't be made at that size
    if (m_extent.width == 0 || m_extent.height == 0)
    {
        return false;
    }

    m_minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0)
    {
        m_minImageCount = std::min(m_minImageCount, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR info = {};
    info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface                  = m_surface;
    info.minImageCount            = m_minImageCount;
    info.imageFormat              = m_format.format;
    info.imageColorSpace          = m_format.colorSpace;
    info.imageExtent              = m_extent;
    info.imageArrayLayers         = 1;
    info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform             = capabilities.currentTransform;
    info.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode              = presentMode;
    info.clipped                  = VK_TRUE;
    info.oldSwapchain             = m_swapchain;

    VkSwapchainKHR swapchain;
    VkResult result = m_vk->vkCreateSwapchainKHR(m_device, &info, nullptr, &swapchain);
    if (result != VK_SUCCESS)
    {
        // This can fail for a few frames while Windows switches fullscreen modes, the retired swapchain makes the next acquire retry
        Logger::LogWarn("vkCreateSwapchainKHR failed with %d", (int) result);
        return false;
    }

    Destroy();
    m_swapchain = swapchain;

    uint32_t imageCount = 0;
    m_vk->vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    m_vk->vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_images.data());

    m_views.resize(imageCount);
    m_framebuffers.resize(imageCount);
    m_renderFinished.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo       = {};
        viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                       = m_images[i];
        viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                      = m_format.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (m_vk->vkCreateImageView(m_device, &viewInfo, nullptr, &m_views[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateImageView failed for the swapchain");
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass              = m_renderPass;
        framebufferInfo.attachmentCount         = 1;
        framebufferInfo.pAttachments            = &m_views[i];
        framebufferInfo.width                   = m_extent.width;
        framebufferInfo.height                  = m_extent.height;
        framebufferInfo.layers                  = 1;
        if (m_vk->vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateFramebuffer failed for the swapchain");
        }

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (m_vk->vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateSemaphore failed for the swapchain");
        }
    }

    Logger::LogInfo("Vulkan swapchain created: %ux%u, %u images, present mode %d, vsync %d", m_extent.width, m_extent.height, imageCount,
                    (int) presentMode, (int) vsync);
    return true;
}

void VkSwapchain::Destroy()
{
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        m_vk->vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
        m_vk->vkDestroyImageView(m_device, m_views[i], nullptr);
        m_vk->vkDestroySemaphore(m_device, m_renderFinished[i], nullptr);
    }

    m_framebuffers.clear();
    m_views.clear();
    m_renderFinished.clear();
    m_images.clear();

    if (m_swapchain != VK_NULL_HANDLE)
    {
        m_vk->vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void VkSwapchain::Shutdown()
{
    Destroy();
    m_vk->vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

VkResult VkSwapchain::Acquire(VkSemaphore semaphore, uint32_t *imageIndex)
{
    return m_vk->vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, imageIndex);
}

VkResult VkSwapchain::Present(VkQueue queue, uint32_t imageIndex)
{
    VkPresentInfoKHR info   = {};
    info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores    = &m_renderFinished[imageIndex];
    info.swapchainCount     = 1;
    info.pSwapchains        = &m_swapchain;
    info.pImageIndices      = &imageIndex;
    return m_vk->vkQueuePresentKHR(queue, &info);
}

VkSwapchainKHR VkSwapchain::GetHandle() const
{
    return m_swapchain;
}

VkRenderPass VkSwapchain::GetRenderPass() const
{
    return m_renderPass;
}

VkExtent2D VkSwapchain::GetExtent() const
{
    return m_extent;
}

VkImage VkSwapchain::GetImage(uint32_t index) const
{
    return m_images[index];
}

VkFramebuffer VkSwapchain::GetFramebuffer(uint32_t index) const
{
    return m_framebuffers[index];
}

VkSemaphore VkSwapchain::GetRenderFinished(uint32_t index) const
{
    return m_renderFinished[index];
}

uint32_t VkSwapchain::GetMinImageCount() const
{
    return m_minImageCount;
}

uint32_t VkSwapchain::GetImageCount() const
{
    return (uint32_t) m_images.size();
}

#endif
