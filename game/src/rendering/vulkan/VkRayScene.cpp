#ifndef EVERSTEAD_DEDICATED_SERVER

#include "VkRayScene.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

VkRayScene::VkRayScene()
{
    m_context        = nullptr;
    m_allocator      = nullptr;
    m_blasPool       = {};
    m_tlasBuffer     = {};
    m_scratch        = {};
    m_inputs[0]      = {};
    m_inputs[1]      = {};
    m_scratchBase    = 0;
    m_blasScratch    = 0;
    m_tlas           = VK_NULL_HANDLE;
    m_instancesDirty = false;
}

void VkRayScene::Init(VkContext *context, VkAllocator *allocator)
{
    m_context      = context;
    m_allocator    = allocator;
    VkDispatch &vk = context->vk;

    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationProperties = {};
    accelerationProperties.sType           = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 properties = {};
    properties.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext                       = &accelerationProperties;
    vk.vkGetPhysicalDeviceProperties2(context->physicalDevice, &properties);
    VkDeviceSize scratchAlignment = accelerationProperties.minAccelerationStructureScratchOffsetAlignment;

    VkAccelerationStructureGeometryKHR geometry = {};
    geometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType                       = VK_GEOMETRY_TYPE_AABBS_KHR;
    geometry.flags                              = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
    geometry.geometry.aabbs.sType               = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.geometry.aabbs.stride              = sizeof(VkAabbPositionsKHR);

    VkAccelerationStructureBuildGeometryInfoKHR build = {};
    build.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build.type                                        = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build.flags                                       = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build.mode                                        = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build.geometryCount                               = 1;
    build.pGeometries                                 = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR blasSizes = {};
    blasSizes.sType                                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vk.vkGetAccelerationStructureBuildSizesKHR(context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &kRayBoxes, &blasSizes);

    VkAccelerationStructureGeometryKHR instances = {};
    instances.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    instances.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    instances.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    build.type                                   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build.pGeometries                            = &instances;

    VkAccelerationStructureBuildSizesInfoKHR tlasSizes = {};
    tlasSizes.sType                                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vk.vkGetAccelerationStructureBuildSizesKHR(context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build, &kRayGeometries, &tlasSizes);

    // Acceleration structure offsets inside a buffer have to be multiples of 256
    VkDeviceSize blasStride           = (blasSizes.accelerationStructureSize + 255) & ~(VkDeviceSize) 255;
    m_blasScratch                     = (blasSizes.buildScratchSize + scratchAlignment - 1) / scratchAlignment * scratchAlignment;
    const VkBufferUsageFlags kStorage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    m_blasPool                        = CreateBuffer(blasStride * kRayGeometries, kStorage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_tlasBuffer                      = CreateBuffer(tlasSizes.accelerationStructureSize, kStorage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_scratch     = CreateBuffer(m_blasScratch * kMaxBuildsPerFrame + tlasSizes.buildScratchSize + scratchAlignment,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_scratchBase = (m_scratch.address + scratchAlignment - 1) / scratchAlignment * scratchAlignment;
    for (int i = 0; i < 2; i++)
    {
        m_inputs[i] = CreateBuffer(kRayGeometries * sizeof(VkAccelerationStructureInstanceKHR) +
                                           kMaxBuildsPerFrame * kRayBoxes * sizeof(VkAabbPositionsKHR),
                                   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    VkAccelerationStructureCreateInfoKHR info = {};
    info.sType                                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    info.buffer                               = m_blasPool.buffer;
    info.size                                 = blasSizes.accelerationStructureSize;
    info.type                                 = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    m_blas.resize(kRayGeometries);
    m_blasAddresses.resize(kRayGeometries);
    for (uint32_t i = 0; i < kRayGeometries; i++)
    {
        info.offset = blasStride * i;
        if (vk.vkCreateAccelerationStructureKHR(context->device, &info, nullptr, &m_blas[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("vkCreateAccelerationStructureKHR failed");
        }

        VkAccelerationStructureDeviceAddressInfoKHR address = {};
        address.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        address.accelerationStructure                       = m_blas[i];
        m_blasAddresses[i]                                  = vk.vkGetAccelerationStructureDeviceAddressKHR(context->device, &address);
    }

    info.buffer = m_tlasBuffer.buffer;
    info.offset = 0;
    info.size   = tlasSizes.accelerationStructureSize;
    info.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    if (vk.vkCreateAccelerationStructureKHR(context->device, &info, nullptr, &m_tlas) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateAccelerationStructureKHR failed");
    }

    m_boxes.resize(kRayGeometries * kRayBoxes);
    m_queued.assign(kRayGeometries, false);
    m_built.assign(kRayGeometries, false);
    // Shaders can bind the scene before anything is added, so the first build makes an empty one
    m_instancesDirty = true;
}

void VkRayScene::Shutdown()
{
    VkDispatch &vk = m_context->vk;
    for (size_t i = 0; i < m_blas.size(); i++)
    {
        vk.vkDestroyAccelerationStructureKHR(m_context->device, m_blas[i], nullptr);
    }

    vk.vkDestroyAccelerationStructureKHR(m_context->device, m_tlas, nullptr);
    DestroyBuffer(m_blasPool);
    DestroyBuffer(m_tlasBuffer);
    DestroyBuffer(m_scratch);
    DestroyBuffer(m_inputs[0]);
    DestroyBuffer(m_inputs[1]);
    m_blas.clear();
}

VkRayScene::Buffer VkRayScene::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags)
{
    VkDispatch &vk = m_context->vk;

    VkBufferCreateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size               = size;
    info.usage              = usage;
    info.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    Buffer buffer;
    if (vk.vkCreateBuffer(m_context->device, &info, nullptr, &buffer.buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("vkCreateBuffer failed for the ray scene");
    }

    VkMemoryRequirements requirements;
    vk.vkGetBufferMemoryRequirements(m_context->device, buffer.buffer, &requirements);
    buffer.allocation = m_allocator->Allocate(requirements, flags, true);
    vk.vkBindBufferMemory(m_context->device, buffer.buffer, buffer.allocation.memory, buffer.allocation.offset);

    VkBufferDeviceAddressInfo address = {};
    address.sType                     = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    address.buffer                    = buffer.buffer;
    buffer.address                    = vk.vkGetBufferDeviceAddress(m_context->device, &address);
    return buffer;
}

void VkRayScene::DestroyBuffer(const Buffer &buffer)
{
    m_context->vk.vkDestroyBuffer(m_context->device, buffer.buffer, nullptr);
    m_allocator->Free(buffer.allocation);
}

void VkRayScene::SetBoxes(uint32_t geometry, const float *boxes)
{
    memcpy(&m_boxes[geometry * kRayBoxes], boxes, kRayBoxes * sizeof(VkAabbPositionsKHR));
    if (!m_queued[geometry])
    {
        m_queued[geometry] = true;
        m_pending.push_back(geometry);
    }
}

void VkRayScene::SetInstances(const RayInstance *instances, uint32_t count)
{
    m_instances.assign(instances, instances + count);
    m_instancesDirty = true;
}

void VkRayScene::Build(VkCommandBuffer cmd, uint32_t frameIndex)
{
    if (m_pending.empty() && !m_instancesDirty)
    {
        return;
    }

    VkDispatch &vk         = m_context->vk;
    const Buffer &input    = m_inputs[frameIndex];
    VkDeviceSize boxOffset = kRayGeometries * sizeof(VkAccelerationStructureInstanceKHR);

    VkMemoryBarrier barrier = {};
    barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    uint32_t builds = std::min((uint32_t) m_pending.size(), kMaxBuildsPerFrame);
    if (builds > 0)
    {
        std::vector<VkAccelerationStructureGeometryKHR> geometries(builds);
        std::vector<VkAccelerationStructureBuildGeometryInfoKHR> infos(builds);
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(builds);
        std::vector<const VkAccelerationStructureBuildRangeInfoKHR *> rangePointers(builds);
        for (uint32_t i = 0; i < builds; i++)
        {
            uint32_t geometry   = m_pending.back();
            VkDeviceSize offset = boxOffset + i * kRayBoxes * sizeof(VkAabbPositionsKHR);
            m_pending.pop_back();
            m_queued[geometry] = false;
            m_built[geometry]  = true;
            memcpy(input.allocation.mapped + offset, &m_boxes[geometry * kRayBoxes], kRayBoxes * sizeof(VkAabbPositionsKHR));

            geometries[i].sType                             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometries[i].geometryType                      = VK_GEOMETRY_TYPE_AABBS_KHR;
            geometries[i].flags                             = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
            geometries[i].geometry.aabbs.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
            geometries[i].geometry.aabbs.data.deviceAddress = input.address + offset;
            geometries[i].geometry.aabbs.stride             = sizeof(VkAabbPositionsKHR);

            infos[i].sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            infos[i].type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            infos[i].flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            infos[i].mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            infos[i].dstAccelerationStructure  = m_blas[geometry];
            infos[i].geometryCount             = 1;
            infos[i].pGeometries               = &geometries[i];
            infos[i].scratchData.deviceAddress = m_scratchBase + i * m_blasScratch;

            ranges[i].primitiveCount = kRayBoxes;
            rangePointers[i]         = &ranges[i];
        }

        vk.vkCmdBuildAccelerationStructuresKHR(cmd, builds, infos.data(), rangePointers.data());
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    VkAccelerationStructureInstanceKHR *written = (VkAccelerationStructureInstanceKHR *) input.allocation.mapped;
    uint32_t count                              = 0;
    for (size_t i = 0; i < m_instances.size(); i++)
    {
        const RayInstance &instance = m_instances[i];
        if (!m_built[instance.geometry])
        {
            continue;
        }

        VkAccelerationStructureInstanceKHR &out = written[count++];
        out                                     = {};
        out.transform.matrix[0][0]              = 1.0f;
        out.transform.matrix[1][1]              = 1.0f;
        out.transform.matrix[2][2]              = 1.0f;
        out.transform.matrix[0][3]              = instance.x;
        out.transform.matrix[1][3]              = instance.y;
        out.transform.matrix[2][3]              = instance.z;
        out.instanceCustomIndex                 = instance.customIndex;
        out.mask                                = 0xFF;
        out.accelerationStructureReference      = m_blasAddresses[instance.geometry];
    }

    VkAccelerationStructureGeometryKHR geometry    = {};
    geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.geometry.instances.data.deviceAddress = input.address;

    VkAccelerationStructureBuildGeometryInfoKHR info = {};
    info.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    info.type                                        = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    info.flags                                       = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    info.mode                                        = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    info.dstAccelerationStructure                    = m_tlas;
    info.geometryCount                               = 1;
    info.pGeometries                                 = &geometry;
    info.scratchData.deviceAddress                   = m_scratchBase + kMaxBuildsPerFrame * m_blasScratch;

    VkAccelerationStructureBuildRangeInfoKHR range               = {};
    range.primitiveCount                                         = count;
    const VkAccelerationStructureBuildRangeInfoKHR *rangePointer = &range;
    vk.vkCmdBuildAccelerationStructuresKHR(cmd, 1, &info, &rangePointer);

    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vk.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &barrier, 0,
                            nullptr, 0, nullptr);
    // Instances whose boxes are still waiting get added once a later frame builds them
    m_instancesDirty = !m_pending.empty();
}

VkAccelerationStructureKHR VkRayScene::GetScene() const
{
    return m_tlas;
}

#endif
