#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkPipelineStore.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>

#include "../../core/Logger.h"
#include "../../io/FileSystem.h"

bool VkPipelineKey::operator==(const VkPipelineKey &other) const
{
    return memcmp(this, &other, sizeof(VkPipelineKey)) == 0;
}

size_t VkPipelineKeyHash::operator()(const VkPipelineKey &key) const
{
    const uint8_t *bytes = (const uint8_t *) &key;
    uint64_t hash        = 14695981039346656037ull;
    for (size_t i = 0; i < sizeof(VkPipelineKey); i++)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }

    return (size_t) hash;
}

VkPipelineStore::VkPipelineStore()
{
    m_vk       = nullptr;
    m_device   = VK_NULL_HANDLE;
    m_features = {};
    m_cache    = VK_NULL_HANDLE;
}

void VkPipelineStore::Init(VkDispatch *vk, VkDevice device, const VkFeatures &features)
{
    m_vk       = vk;
    m_device   = device;
    m_features = features;

    File cacheFile = FileSystem::GetUserDirectory().GetChild("pipelines.cache");
    std::vector<uint8_t> data;
    if (cacheFile.Exists())
    {
        data = cacheFile.ReadAllBytes();
    }

    // Drivers ignore cache data from another device or driver version, so a stale file only costs the speedup
    VkPipelineCacheCreateInfo info = {};
    info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    info.initialDataSize           = data.size();
    info.pInitialData              = data.data();
    if (m_vk->vkCreatePipelineCache(m_device, &info, nullptr, &m_cache) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreatePipelineCache failed");
    }

    LoadRecorded();
    size_t keyCount = 0;
    for (std::pair<const std::string, std::vector<VkPipelineKey>> &entry : m_recorded)
    {
        keyCount += entry.second.size();
    }

    Logger::LogInfo("Pipeline cache loaded %zu bytes and %zu recorded keys for %zu programs", data.size(), keyCount, m_recorded.size());
}

void VkPipelineStore::Shutdown()
{
    size_t size = 0;
    m_vk->vkGetPipelineCacheData(m_device, m_cache, &size, nullptr);
    std::vector<uint8_t> data(size);
    if (m_vk->vkGetPipelineCacheData(m_device, m_cache, &size, data.data()) == VK_SUCCESS)
    {
        data.resize(size);
        FileSystem::GetUserDirectory().GetChild("pipelines.cache").WriteAllBytes(data);
        Logger::LogInfo("Saved %zu bytes of pipeline cache for %zu pipelines", size, m_pipelines.size());
    }

    SaveRecorded();

    for (std::pair<const VkPipelineKey, VkPipeline> &entry : m_pipelines)
    {
        m_vk->vkDestroyPipeline(m_device, entry.second, nullptr);
    }

    for (std::pair<const VkFormat, VkRenderPass> &entry : m_renderPasses)
    {
        m_vk->vkDestroyRenderPass(m_device, entry.second, nullptr);
    }

    m_vk->vkDestroyPipelineCache(m_device, m_cache, nullptr);
    m_pipelines.clear();
    m_renderPasses.clear();
}

void VkPipelineStore::LoadRecorded()
{
    File keysFile = FileSystem::GetUserDirectory().GetChild("pipelines.keys");
    if (!keysFile.Exists())
    {
        return;
    }

    std::vector<uint8_t> data = keysFile.ReadAllBytes();
    uint32_t keySize          = 0;
    if (data.size() >= 4)
    {
        memcpy(&keySize, data.data(), 4);
    }

    if (keySize != sizeof(VkPipelineKey))
    {
        Logger::LogWarn("Ignoring recorded pipeline keys with key size %u, expected %zu", keySize, sizeof(VkPipelineKey));
        return;
    }

    size_t offset = 4;
    while (offset + 4 <= data.size())
    {
        uint32_t nameLength;
        memcpy(&nameLength, data.data() + offset, 4);
        offset += 4;
        if (offset + nameLength + 4 > data.size())
        {
            break;
        }

        std::string name((const char *) data.data() + offset, nameLength);
        offset += nameLength;
        uint32_t count;
        memcpy(&count, data.data() + offset, 4);
        offset += 4;
        if (count > (data.size() - offset) / sizeof(VkPipelineKey))
        {
            break;
        }

        std::vector<VkPipelineKey> &keys = m_recorded[name];
        for (uint32_t i = 0; i < count; i++)
        {
            VkPipelineKey key;
            memcpy(&key, data.data() + offset, sizeof(key));
            offset += sizeof(key);
            if (key.topology < std::size(kTopologies) && key.depthFunc < std::size(kCompareOps) && key.srcFactor < 7 && key.dstFactor < 7 &&
                key.cullFace < std::size(kCullModes) && key.frontFace < std::size(kFrontFaces))
            {
                keys.push_back(key);
            }
        }
    }
}

void VkPipelineStore::SaveRecorded()
{
    uint32_t keySize = sizeof(VkPipelineKey);
    std::vector<uint8_t> data((const uint8_t *) &keySize, (const uint8_t *) &keySize + 4);
    for (std::pair<const std::string, std::vector<VkPipelineKey>> &entry : m_recorded)
    {
        uint32_t nameLength = (uint32_t) entry.first.size();
        uint32_t count      = (uint32_t) entry.second.size();
        data.insert(data.end(), (const uint8_t *) &nameLength, (const uint8_t *) &nameLength + 4);
        data.insert(data.end(), entry.first.begin(), entry.first.end());
        data.insert(data.end(), (const uint8_t *) &count, (const uint8_t *) &count + 4);
        data.insert(data.end(), (const uint8_t *) entry.second.data(), (const uint8_t *) (entry.second.data() + count));
    }

    FileSystem::GetUserDirectory().GetChild("pipelines.keys").WriteAllBytes(data);
}

VkRenderPass VkPipelineStore::GetRenderPass(VkFormat format)
{
    auto it = m_renderPasses.find(format);
    if (it != m_renderPasses.end())
    {
        return it->second;
    }

    VkAttachmentDescription attachments[2] = {};
    VkFormat formats[2]                    = {format, VK_FORMAT_D32_SFLOAT};
    VkImageLayout layouts[2]               = {VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    for (int i = 0; i < 2; i++)
    {
        attachments[i].format         = formats[i];
        attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[i].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout  = layouts[i];
        attachments[i].finalLayout    = layouts[i];
    }

    VkAttachmentReference colorReference = {0, layouts[0]};
    VkAttachmentReference depthReference = {1, layouts[1]};

    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorReference;
    subpass.pDepthStencilAttachment = &depthReference;

    VkRenderPassCreateInfo info = {};
    info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount        = 2;
    info.pAttachments           = attachments;
    info.subpassCount           = 1;
    info.pSubpasses             = &subpass;

    VkRenderPass renderPass;
    if (m_vk->vkCreateRenderPass(m_device, &info, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateRenderPass failed");
    }

    Logger::LogInfo("Created render pass for color format %d", (int) format);

    m_renderPasses[format] = renderPass;
    return renderPass;
}

VkPipeline VkPipelineStore::Get(const VkPipelineKey &key, const VkProgramData &program)
{
    auto it = m_pipelines.find(key);
    if (it != m_pipelines.end())
    {
        return it->second;
    }

    VkPipeline pipeline = Create(key, program);
    Logger::LogInfo("Created pipeline for %s at draw time, %zu pipelines total", program.name.c_str(), m_pipelines.size());
    VkPipelineKey recorded           = key;
    recorded.program                 = 0;
    std::vector<VkPipelineKey> &keys = m_recorded[program.name];
    if (std::find(keys.begin(), keys.end(), recorded) == keys.end())
    {
        keys.push_back(recorded);
    }

    return pipeline;
}

void VkPipelineStore::Prewarm(uint32_t programId, const VkProgramData &program)
{
    auto it = m_recorded.find(program.name);
    if (it == m_recorded.end())
    {
        return;
    }

    std::vector<VkPipelineKey> &keys = it->second;
    size_t kept                      = 0;
    for (size_t i = 0; i < keys.size(); i++)
    {
        // A shader edit can change the inputs, which makes the recorded vertex layout invalid for it
        bool matches = true;
        for (uint32_t location = 0; location < 16; location++)
        {
            if (((program.inputMask >> location) & 1) != (keys[i].formats[location] != VK_FORMAT_UNDEFINED))
            {
                matches = false;
            }
        }

        if (!matches)
        {
            continue;
        }

        keys[kept] = keys[i];
        kept++;
        VkPipelineKey key = keys[i];
        key.program       = programId;
        if (m_pipelines.find(key) == m_pipelines.end())
        {
            Create(key, program);
        }
    }

    if (kept != keys.size())
    {
        Logger::LogWarn("Dropped %zu recorded pipelines for %s because its inputs changed", keys.size() - kept, program.name.c_str());
    }

    keys.resize(kept);
    Logger::LogInfo("Prewarmed %zu pipelines for %s", kept, program.name.c_str());
}

VkPipeline VkPipelineStore::Create(const VkPipelineKey &key, const VkProgramData &program)
{
    const VkBlendFactor kBlendFactors[] = {VK_BLEND_FACTOR_ZERO,      VK_BLEND_FACTOR_ONE,
                                           VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                           VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
                                           VK_BLEND_FACTOR_DST_COLOR};

    VkVertexInputBindingDescription bindings[16];
    VkVertexInputAttributeDescription attributes[16];
    uint32_t attributeCount = 0;
    for (uint32_t location = 0; location < 16; location++)
    {
        if (!(program.inputMask & (1 << location)))
        {
            continue;
        }

        bindings[attributeCount].binding    = location;
        bindings[attributeCount].stride     = key.strides[location];
        bindings[attributeCount].inputRate  = key.instanceMask & (1 << location) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        attributes[attributeCount].location = location;
        attributes[attributeCount].binding  = location;
        attributes[attributeCount].format   = key.formats[location];
        attributes[attributeCount].offset   = 0;
        attributeCount++;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage                           = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module                          = program.vertex;
    stages[0].pName                           = "main";
    VkBool32 remapDepth                       = !m_features.depthClipControl;
    VkSpecializationMapEntry remapDepthEntry  = {0, 0, sizeof(VkBool32)};
    VkSpecializationInfo specialization       = {1, &remapDepthEntry, sizeof(VkBool32), &remapDepth};
    stages[0].pSpecializationInfo             = &specialization;
    stages[1].sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module                          = program.fragment;
    stages[1].pName                           = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount        = attributeCount;
    vertexInput.pVertexBindingDescriptions           = bindings;
    vertexInput.vertexAttributeDescriptionCount      = attributeCount;
    vertexInput.pVertexAttributeDescriptions         = attributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = kTopologies[key.topology];

    VkPipelineViewportDepthClipControlCreateInfoEXT depthClipControl = {};
    depthClipControl.sType                                           = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT;
    depthClipControl.negativeOneToOne                                = VK_TRUE;

    VkPipelineViewportStateCreateInfo viewport = {};
    viewport.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.pNext                             = m_features.depthClipControl ? &depthClipControl : nullptr;
    viewport.viewportCount                     = 1;
    viewport.scissorCount                      = 1;

    VkPipelineRasterizationStateCreateInfo rasterization = {};
    rasterization.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode                            = key.polygonMode == ePolygonMode_Line ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterization.cullMode                               = key.cull ? kCullModes[key.cullFace] : VK_CULL_MODE_NONE;
    rasterization.frontFace                              = kFrontFaces[key.frontFace];
    rasterization.depthBiasEnable                        = key.depthBias;
    rasterization.lineWidth                              = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable                       = key.depthTest;
    depthStencil.depthWriteEnable                      = key.depthTest && key.depthWrite;
    depthStencil.depthCompareOp                        = kCompareOps[key.depthFunc];

    VkPipelineColorBlendAttachmentState blendAttachment = {};
    blendAttachment.blendEnable                         = key.blend;
    blendAttachment.srcColorBlendFactor                 = kBlendFactors[key.srcFactor];
    blendAttachment.dstColorBlendFactor                 = kBlendFactors[key.dstFactor];
    blendAttachment.colorBlendOp                        = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor                 = kBlendFactors[key.srcFactor];
    blendAttachment.dstAlphaBlendFactor                 = kBlendFactors[key.dstFactor];
    blendAttachment.alphaBlendOp                        = VK_BLEND_OP_ADD;
    blendAttachment.colorWriteMask                      = key.colorMask;

    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    colorBlend.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.logicOpEnable                       = key.logicOp != eLogicOp_None;
    colorBlend.logicOp                             = VK_LOGIC_OP_OR_REVERSE;
    colorBlend.attachmentCount                     = 1;
    colorBlend.pAttachments                        = &blendAttachment;

    VkDynamicState dynamicStates[16];
    uint32_t dynamicCount         = 0;
    dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    if (m_features.wideLines)
    {
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_LINE_WIDTH;
    }

    if (m_features.extendedDynamicState)
    {
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_CULL_MODE_EXT;
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_FRONT_FACE_EXT;
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT;
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT;
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT;
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT;
    }

    if (m_features.extendedDynamicState2)
    {
        dynamicStates[dynamicCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT;
    }

    VkPipelineDynamicStateCreateInfo dynamic = {};
    dynamic.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount                = dynamicCount;
    dynamic.pDynamicStates                   = dynamicStates;

    VkGraphicsPipelineCreateInfo info = {};
    info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount                   = 2;
    info.pStages                      = stages;
    info.pVertexInputState            = &vertexInput;
    info.pInputAssemblyState          = &inputAssembly;
    info.pViewportState               = &viewport;
    info.pRasterizationState          = &rasterization;
    info.pMultisampleState            = &multisample;
    info.pDepthStencilState           = &depthStencil;
    info.pColorBlendState             = &colorBlend;
    info.pDynamicState                = &dynamic;
    info.layout                       = program.pipelineLayout;

    VkPipelineRenderingCreateInfoKHR rendering = {};
    rendering.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rendering.colorAttachmentCount             = 1;
    rendering.pColorAttachmentFormats          = &key.colorFormat;
    rendering.depthAttachmentFormat            = VK_FORMAT_D32_SFLOAT;
    if (m_features.dynamicRendering)
    {
        info.pNext = &rendering;
    }
    else
    {
        info.renderPass = GetRenderPass(key.colorFormat);
    }

    VkPipeline pipeline;
    if (m_vk->vkCreateGraphicsPipelines(m_device, m_cache, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateGraphicsPipelines failed");
    }

    m_pipelines[key] = pipeline;
    return pipeline;
}

void VkPipelineStore::RemoveProgram(uint32_t program, VkDeletionQueue *deletionQueue)
{
    auto it = m_pipelines.begin();
    while (it != m_pipelines.end())
    {
        if (it->first.program == program)
        {
            deletionQueue->PushPipeline(it->second);
            it = m_pipelines.erase(it);
        }
        else
        {
            it++;
        }
    }
}

#endif
