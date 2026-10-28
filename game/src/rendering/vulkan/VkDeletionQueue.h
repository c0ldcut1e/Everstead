#pragma once

#include <vector>

#include "VkAllocator.h"
#include "VkDispatch.h"

class VkDeletionQueue
{
public:
    void PushBuffer(VkBuffer buffer, const VkAllocation &allocation);
    void PushImage(VkImage image, VkImageView view, const VkAllocation &allocation);
    void PushFramebuffer(VkFramebuffer framebuffer);
    void PushPipeline(VkPipeline pipeline);
    void PushProgram(VkShaderModule vertex, VkShaderModule fragment, VkDescriptorSetLayout uniformLayout, VkDescriptorSetLayout samplerLayout,
                     VkPipelineLayout pipelineLayout);
    void Flush(VkDispatch *vk, VkDevice device, VkAllocator *allocator);

private:
    std::vector<VkBuffer> m_buffers;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    std::vector<VkAllocation> m_allocations;
    std::vector<VkFramebuffer> m_framebuffers;
    std::vector<VkPipeline> m_pipelines;
    std::vector<VkShaderModule> m_shaderModules;
    std::vector<VkDescriptorSetLayout> m_setLayouts;
    std::vector<VkPipelineLayout> m_pipelineLayouts;
};
