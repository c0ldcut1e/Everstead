#pragma once

#include <cstdint>
#include <vector>

#include "VkDispatch.h"

class VkSwapchain
{
public:
    VkSwapchain();

    void Init(VkDispatch *vk, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface);
    bool Create(uint32_t width, uint32_t height, bool vsync);
    void Destroy();
    void Shutdown();

    VkResult Acquire(VkSemaphore semaphore, uint32_t *imageIndex);
    VkResult Present(VkQueue queue, uint32_t imageIndex);

    VkSwapchainKHR GetHandle() const;
    VkRenderPass GetRenderPass() const;
    VkExtent2D GetExtent() const;
    VkImage GetImage(uint32_t index) const;
    VkFramebuffer GetFramebuffer(uint32_t index) const;
    VkSemaphore GetRenderFinished(uint32_t index) const;
    uint32_t GetMinImageCount() const;
    uint32_t GetImageCount() const;

private:
    void CreateRenderPass();

    VkDispatch *m_vk;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkSurfaceFormatKHR m_format;
    VkRenderPass m_renderPass;
    VkSwapchainKHR m_swapchain;
    VkExtent2D m_extent;
    uint32_t m_minImageCount;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkSemaphore> m_renderFinished;
};
