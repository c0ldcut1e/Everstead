#pragma once

struct VkFeatures
{
    bool wideLines;
    bool samplerAnisotropy;
    bool memoryBudget;
    bool debugUtils;
    bool descriptorIndexing;
    bool timelineSemaphore;
    bool bufferDeviceAddress;
    bool drawIndirectCount;
    bool dynamicRendering;
    bool synchronization2;
    bool extendedDynamicState;
    bool extendedDynamicState2;
    bool pushDescriptor;
    bool depthClipControl;
    bool rayQuery;
};
