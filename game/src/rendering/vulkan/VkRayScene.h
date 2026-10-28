#pragma once

#include <cstdint>
#include <vector>

#include "../RenderTypes.h"
#include "VkAllocator.h"
#include "VkContext.h"

class VkRayScene
{
public:
    VkRayScene();

    void Init(VkContext *context, VkAllocator *allocator);
    void Shutdown();

    void SetBoxes(uint32_t geometry, const float *boxes);
    void SetInstances(const RayInstance *instances, uint32_t count);
    void Build(VkCommandBuffer cmd, uint32_t frameIndex);
    VkAccelerationStructureKHR GetScene() const;

private:
    struct Buffer
    {
        VkBuffer buffer;
        VkAllocation allocation;
        VkDeviceAddress address;
    };

    static constexpr uint32_t kMaxBuildsPerFrame = 256;

    Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags);
    void DestroyBuffer(const Buffer &buffer);

    VkContext *m_context;
    VkAllocator *m_allocator;
    Buffer m_blasPool;
    Buffer m_tlasBuffer;
    Buffer m_scratch;
    Buffer m_inputs[2];
    VkDeviceAddress m_scratchBase;
    VkDeviceSize m_blasScratch;
    std::vector<VkAccelerationStructureKHR> m_blas;
    std::vector<VkDeviceAddress> m_blasAddresses;
    VkAccelerationStructureKHR m_tlas;
    std::vector<VkAabbPositionsKHR> m_boxes;
    std::vector<uint32_t> m_pending;
    std::vector<bool> m_queued;
    std::vector<bool> m_built;
    std::vector<RayInstance> m_instances;
    bool m_instancesDirty;
};
