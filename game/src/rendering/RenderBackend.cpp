#ifndef EVERSTEAD_DEDICATED_SERVER

#include "RenderBackend.h"

#include "opengl/GlRenderDevice.h"
#include "vulkan/VkRenderDevice.h"

RenderDevice *RenderBackend::CreateDevice(eRENDER_BACKEND backend)
{
    switch (backend)
    {
        case eRenderBackend_OpenGL: {
            return new GlRenderDevice();
        }
        case eRenderBackend_Vulkan: {
            return new VkRenderDevice();
        }
    }

    return nullptr;
}

int RenderBackend::FindIndex(const std::string &key)
{
    for (int i = 0; i < kBackendCount; i++)
    {
        if (key == kBackends[i].key)
        {
            return i;
        }
    }

    return 0;
}

#endif
