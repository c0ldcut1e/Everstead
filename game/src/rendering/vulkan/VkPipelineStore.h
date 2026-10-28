#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "VkDeletionQueue.h"
#include "VkDispatch.h"
#include "VkFeatures.h"
#include "VkResources.h"

constexpr VkPrimitiveTopology kTopologies[] = {VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
                                               VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN};
constexpr VkCompareOp kCompareOps[]         = {VK_COMPARE_OP_LESS, VK_COMPARE_OP_LESS_OR_EQUAL, VK_COMPARE_OP_EQUAL, VK_COMPARE_OP_GREATER};
constexpr VkCullModeFlagBits kCullModes[]   = {VK_CULL_MODE_BACK_BIT};
// The backbuffer is stored bottom row first, which mirrors the winding, so counter clockwise maps to clockwise
constexpr VkFrontFace kFrontFaces[] = {VK_FRONT_FACE_CLOCKWISE};

struct VkPipelineKey
{
    uint32_t program;
    VkFormat colorFormat;
    VkFormat formats[16];
    uint32_t strides[16];
    uint32_t instanceMask;
    uint8_t topology;
    uint8_t depthTest;
    uint8_t depthWrite;
    uint8_t depthFunc;
    uint8_t cull;
    uint8_t cullFace;
    uint8_t frontFace;
    uint8_t blend;
    uint8_t srcFactor;
    uint8_t dstFactor;
    uint8_t colorMask;
    uint8_t depthBias;
    uint8_t polygonMode;
    uint8_t logicOp;

    bool operator==(const VkPipelineKey &other) const;
};

struct VkPipelineKeyHash
{
    size_t operator()(const VkPipelineKey &key) const;
};

class VkPipelineStore
{
public:
    VkPipelineStore();

    void Init(VkDispatch *vk, VkDevice device, const VkFeatures &features);
    void Shutdown();

    VkRenderPass GetRenderPass(VkFormat format);
    VkPipeline Get(const VkPipelineKey &key, const VkProgramData &program);
    void Prewarm(uint32_t programId, const VkProgramData &program);
    void RemoveProgram(uint32_t program, VkDeletionQueue *deletionQueue);

private:
    VkPipeline Create(const VkPipelineKey &key, const VkProgramData &program);
    void LoadRecorded();
    void SaveRecorded();

    VkDispatch *m_vk;
    VkDevice m_device;
    VkFeatures m_features;
    VkPipelineCache m_cache;
    std::unordered_map<VkPipelineKey, VkPipeline, VkPipelineKeyHash> m_pipelines;
    std::unordered_map<VkFormat, VkRenderPass> m_renderPasses;
    std::unordered_map<std::string, std::vector<VkPipelineKey>> m_recorded;
};
