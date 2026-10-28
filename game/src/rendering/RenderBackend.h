#pragma once

#include <cstdint>
#include <iterator>
#include <string>

#include "RenderDevice.h"

class RenderBackend
{
public:
    struct Info
    {
        eRENDER_BACKEND backend;
        const char *key;
        const char *nameKey;
    };

    static RenderDevice *CreateDevice(eRENDER_BACKEND backend);
    static int FindIndex(const std::string &key);

    // A forced platform always runs its first backend and hides the option from the settings menu
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    static constexpr bool kForced     = false;
    static constexpr Info kBackends[] = {{eRenderBackend_Vulkan, "vulkan", "options.graphics.render_backend.vulkan"},
                                         {eRenderBackend_OpenGL, "opengl", "options.graphics.render_backend.opengl"}};
#else
#error "No render backend is set up for this platform"
#endif

    static constexpr int kBackendCount = (int) std::size(kBackends);
};
