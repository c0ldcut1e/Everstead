#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../RenderTypes.h"
#include "VkAllocator.h"
#include "VkDispatch.h"

struct VkUsage
{
    uint64_t submit;
    uint64_t uploadEpoch;
};

struct VkBufferData
{
    uint32_t id;
    VkBuffer buffer;
    VkAllocation allocation;
    VkDeviceSize size;
    VkBuffer streamBuffer;
    VkDeviceSize streamOffset;
    VkUsage usage;
};

struct VkTextureData
{
    uint32_t id;
    VkImage image;
    VkImageView view;
    VkAllocation allocation;
    eTEXTURE_FORMAT format;
    int width;
    int height;
    uint32_t levels;
    uint32_t generation;
    eTEXTURE_FILTER minFilter;
    eTEXTURE_FILTER magFilter;
    eTEXTURE_WRAP wrapS;
    eTEXTURE_WRAP wrapT;
    int maxLevel;
    float anisotropy;
    VkUsage usage;
    VkImageLayout layout;
    VkPipelineStageFlags2KHR stages;
    VkAccessFlags2KHR access;
};

struct VkVertexAttrib
{
    uint32_t buffer;
    VkFormat format;
    uint32_t stride;
    uint32_t offset;
    bool instanced;
};

struct VkVertexArrayData
{
    VkVertexAttrib attribs[16];
    uint32_t mask;
    uint32_t elementBuffer;
};

struct VkUniformBlock
{
    uint32_t binding;
    uint32_t offset;
    uint32_t size;
};

struct VkProgramData
{
    std::string name;
    VkShaderModule vertex;
    VkShaderModule fragment;
    VkDescriptorSetLayout uniformLayout;
    VkDescriptorSetLayout samplerLayout;
    VkPipelineLayout pipelineLayout;
    std::vector<VkUniformBlock> blocks;
    std::vector<uint32_t> bindingOrder;
    std::vector<uint8_t> uniformData;
    uint32_t inputMask;
    uint32_t samplerMask;
    int sceneBinding;
    bool uniformsDirty;
    uint64_t uniformFrame;
    VkDescriptorSet uniformSet;
    VkBuffer uniformBuffer;
    VkDeviceSize uniformBase;
};

struct VkComputeProgramData
{
    VkShaderModule module;
    VkDescriptorSetLayout layout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    uint32_t uniformSize;
    std::vector<uint32_t> samplerBindings;
};

struct VkFramebufferData
{
    uint32_t color;
    uint32_t depth;
    uint32_t colorGeneration;
    uint32_t depthGeneration;
    VkFramebuffer framebuffer;
    VkRenderPass renderPass;
};
