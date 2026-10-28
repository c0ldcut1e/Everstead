#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkDeletionQueue.h"

void VkDeletionQueue::PushBuffer(VkBuffer buffer, const VkAllocation &allocation)
{
    m_buffers.push_back(buffer);
    m_allocations.push_back(allocation);
}

void VkDeletionQueue::PushImage(VkImage image, VkImageView view, const VkAllocation &allocation)
{
    m_images.push_back(image);
    m_imageViews.push_back(view);
    m_allocations.push_back(allocation);
}

void VkDeletionQueue::PushFramebuffer(VkFramebuffer framebuffer)
{
    m_framebuffers.push_back(framebuffer);
}

void VkDeletionQueue::PushPipeline(VkPipeline pipeline)
{
    m_pipelines.push_back(pipeline);
}

void VkDeletionQueue::PushProgram(VkShaderModule vertex, VkShaderModule fragment, VkDescriptorSetLayout uniformLayout,
                                  VkDescriptorSetLayout samplerLayout, VkPipelineLayout pipelineLayout)
{
    m_shaderModules.push_back(vertex);
    m_shaderModules.push_back(fragment);
    m_setLayouts.push_back(uniformLayout);
    m_setLayouts.push_back(samplerLayout);
    m_pipelineLayouts.push_back(pipelineLayout);
}

void VkDeletionQueue::Flush(VkDispatch *vk, VkDevice device, VkAllocator *allocator)
{
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        vk->vkDestroyFramebuffer(device, m_framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < m_imageViews.size(); i++)
    {
        vk->vkDestroyImageView(device, m_imageViews[i], nullptr);
    }

    for (size_t i = 0; i < m_images.size(); i++)
    {
        vk->vkDestroyImage(device, m_images[i], nullptr);
    }

    for (size_t i = 0; i < m_buffers.size(); i++)
    {
        vk->vkDestroyBuffer(device, m_buffers[i], nullptr);
    }

    for (size_t i = 0; i < m_allocations.size(); i++)
    {
        allocator->Free(m_allocations[i]);
    }

    for (size_t i = 0; i < m_pipelines.size(); i++)
    {
        vk->vkDestroyPipeline(device, m_pipelines[i], nullptr);
    }

    for (size_t i = 0; i < m_pipelineLayouts.size(); i++)
    {
        vk->vkDestroyPipelineLayout(device, m_pipelineLayouts[i], nullptr);
    }

    for (size_t i = 0; i < m_setLayouts.size(); i++)
    {
        vk->vkDestroyDescriptorSetLayout(device, m_setLayouts[i], nullptr);
    }

    for (size_t i = 0; i < m_shaderModules.size(); i++)
    {
        vk->vkDestroyShaderModule(device, m_shaderModules[i], nullptr);
    }

    m_framebuffers.clear();
    m_imageViews.clear();
    m_images.clear();
    m_buffers.clear();
    m_allocations.clear();
    m_pipelines.clear();
    m_pipelineLayouts.clear();
    m_setLayouts.clear();
    m_shaderModules.clear();
}

#endif
