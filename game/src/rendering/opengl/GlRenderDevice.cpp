#ifndef EVERSTEAD_DEDICATED_SERVER

#include "GlRenderDevice.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "../../core/FrameProfiler.h"
#include "../../core/Logger.h"
#include "../../io/File.h"
#include "../../memory/MemoryTracker.h"

#include <glad/glad.h>

#include "../../ui/imgui/imgui_impl_opengl3.h"
#include "../../ui/imgui/imgui_impl_sdl2.h"

#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif

#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#endif

#ifndef GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif

#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif

#ifndef GL_PARAMETER_BUFFER
#define GL_PARAMETER_BUFFER 0x80EE
#endif

#ifndef GL_COMMAND_BARRIER_BIT
#define GL_COMMAND_BARRIER_BIT 0x40
#endif

#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT 0x2000
#endif

#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT 0x92E0
#endif

#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#endif

#ifndef GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_TYPE_ERROR 0x824C
#endif

#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#endif

#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#endif

// The glad loader only covers GL 3.3, so newer entry points are fetched by hand
typedef void(APIENTRYP BufferStorageProc)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void(APIENTRYP MultiDrawElementsIndirectProc)(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride);
typedef void(APIENTRYP MultiDrawElementsIndirectCountProc)(GLenum mode, GLenum type, const void *indirect, GLintptr drawCount, GLsizei maxDrawCount,
                                                           GLsizei stride);
typedef void(APIENTRYP ClearBufferDataProc)(GLenum target, GLenum internalFormat, GLenum format, GLenum type, const void *data);
typedef void(APIENTRYP DebugMessageCallbackProc)(GLDEBUGPROC callback, const void *userParam);
typedef void(APIENTRYP DispatchComputeProc)(GLuint groupsX, GLuint groupsY, GLuint groupsZ);
typedef void(APIENTRYP MemoryBarrierProc)(GLbitfield barriers);

static BufferStorageProc s_bufferStorage                                   = nullptr;
static MultiDrawElementsIndirectProc s_multiDrawElementsIndirect           = nullptr;
static MultiDrawElementsIndirectCountProc s_multiDrawElementsIndirectCount = nullptr;
static ClearBufferDataProc s_clearBufferData                               = nullptr;
static bool s_bufferStorageChecked                                         = false;
static DispatchComputeProc s_dispatchCompute                               = nullptr;
static MemoryBarrierProc s_memoryBarrier                                   = nullptr;

static void APIENTRY OnDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
                                    const void *userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    if (type == GL_DEBUG_TYPE_ERROR || severity == GL_DEBUG_SEVERITY_HIGH)
    {
        Logger::LogError("OpenGL: %s", message);
    }
    else
    {
        Logger::LogWarn("OpenGL: %s", message);
    }
}

static const GLenum kPrimitiveTypes[] = {GL_TRIANGLES, GL_LINES, GL_LINE_LOOP, GL_TRIANGLE_FAN};
static const GLenum kCompareFuncs[]   = {GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GREATER};
static const GLenum kBlendFactors[]   = {GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR};
static const GLenum kCullFaces[]      = {GL_BACK};
static const GLenum kFrontFaces[]     = {GL_CCW};
static const GLenum kPolygonModes[]   = {GL_FILL, GL_LINE};
static const GLint kTextureFilters[]  = {GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST};
static const GLint kTextureWraps[]    = {GL_REPEAT, GL_CLAMP_TO_EDGE};
static const GLenum kAttribTypes[]    = {GL_FLOAT, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_BYTE};
static const GLenum kBufferUsages[]   = {GL_STATIC_DRAW, GL_STREAM_DRAW};

struct GlTextureFormat
{
    GLint internalFormat;
    GLenum format;
    GLenum type;
};

static const GlTextureFormat kTextureFormats[] = {
        {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
        {GL_RGBA16F, GL_RGBA, GL_FLOAT},
        {GL_RGBA32F, GL_RGBA, GL_FLOAT},
        {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT},
        {GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT},
};

GlRenderDevice::GlRenderDevice()
{
    m_context            = nullptr;
    m_timerFrame         = 0;
    m_readbackBuffers[0] = 0;
    m_readbackBuffers[1] = 0;
    m_readbackSize       = 0;
    m_readbackTarget     = 1;
    m_readbackFence      = nullptr;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | (getenv("EVERSTEAD_GL_DEBUG") ? SDL_GL_CONTEXT_DEBUG_FLAG : 0));
#else
    if (getenv("EVERSTEAD_GL_DEBUG"))
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

GlRenderDevice::~GlRenderDevice()
{
    if (m_context)
    {
        glDeleteBuffers(2, m_readbackBuffers);
        glDeleteSync((GLsync) m_readbackFence);
        SDL_GL_DeleteContext(m_context);
    }
}

eRENDER_BACKEND GlRenderDevice::GetBackend() const
{
    return eRenderBackend_OpenGL;
}

uint32_t GlRenderDevice::GetWindowFlags() const
{
    return SDL_WINDOW_OPENGL;
}

void GlRenderDevice::CreateContext(SDL_Window *window)
{
    m_context = SDL_GL_CreateContext(window);
    if (!m_context)
    {
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    if (SDL_GL_MakeCurrent(window, m_context) != 0)
    {
        throw std::runtime_error(std::string("SDL_GL_MakeCurrent failed: ") + SDL_GetError());
    }

    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
    {
        throw std::runtime_error("gladLoadGLLoader failed");
    }

    if (!glGetString(GL_VERSION))
    {
        throw std::runtime_error("glGetString(GL_VERSION) failed");
    }

    s_multiDrawElementsIndirect = (MultiDrawElementsIndirectProc) SDL_GL_GetProcAddress("glMultiDrawElementsIndirect");
    if (!s_multiDrawElementsIndirect)
    {
        throw std::runtime_error("glMultiDrawElementsIndirect is missing, OpenGL 4.3 is required");
    }

    if (SDL_GL_ExtensionSupported("GL_ARB_indirect_parameters"))
    {
        s_multiDrawElementsIndirectCount = (MultiDrawElementsIndirectCountProc) SDL_GL_GetProcAddress("glMultiDrawElementsIndirectCountARB");
    }

    s_clearBufferData = (ClearBufferDataProc) SDL_GL_GetProcAddress("glClearBufferData");
    s_dispatchCompute = (DispatchComputeProc) SDL_GL_GetProcAddress("glDispatchCompute");
    s_memoryBarrier   = (MemoryBarrierProc) SDL_GL_GetProcAddress("glMemoryBarrier");

    if (getenv("EVERSTEAD_GL_DEBUG"))
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        ((DebugMessageCallbackProc) SDL_GL_GetProcAddress("glDebugMessageCallback"))(OnDebugMessage, nullptr);
        Logger::LogInfo("OpenGL debug output enabled");
    }

    GLint maxTextureSize      = 0;
    GLint maxVertexAttribs    = 0;
    GLint maxTextureUnits     = 0;
    GLint maxUniformBlockSize = 0;
    GLint maxSamples          = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    Logger::LogInfo("OpenGL context: %s", GetVersion());
    Logger::LogInfo("OpenGL device: %s (%s)", GetRenderer(), GetVendor());
    Logger::LogInfo("OpenGL GLSL: %s", (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION));
    Logger::LogInfo("OpenGL limits: maxTexture=%d maxVertexAttribs=%d textureUnits=%d uniformBlock=%d samples=%d", maxTextureSize, maxVertexAttribs,
                    maxTextureUnits, maxUniformBlockSize, maxSamples);
    Logger::LogInfo("OpenGL extensions: buffer_storage=%d anisotropic=%d nvx_meminfo=%d ati_meminfo=%d",
                    SDL_GL_ExtensionSupported("GL_ARB_buffer_storage"), SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic"),
                    SDL_GL_ExtensionSupported("GL_NVX_gpu_memory_info"), SDL_GL_ExtensionSupported("GL_ATI_meminfo"));

    for (size_t i = 0; i < 4; i++)
    {
        glGenQueries(64, m_timerFrames[i].queries);
        m_timerFrames[i].timing = false;
    }

    BeginTimerFrame();
}

void GlRenderDevice::Present(SDL_Window *window)
{
    EndGpuTimer();
    SDL_GL_SwapWindow(window);
    m_timerFrame = (m_timerFrame + 1) % 4;
    BeginTimerFrame();
}

void GlRenderDevice::BeginTimerFrame()
{
    TimerFrame &frame = m_timerFrames[m_timerFrame];
    if (!frame.names.empty())
    {
        GLuint available = 0;
        glGetQueryObjectuiv(frame.queries[frame.names.size() * 2 - 1], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available)
        {
            for (size_t i = 0; i < frame.names.size(); i++)
            {
                GLuint64 start = 0;
                GLuint64 end   = 0;
                glGetQueryObjectui64v(frame.queries[i * 2], GL_QUERY_RESULT, &start);
                glGetQueryObjectui64v(frame.queries[i * 2 + 1], GL_QUERY_RESULT, &end);
                FrameProfiler::AddGpuTime(frame.names[i], (double) (end - start) / 1e6);
            }
        }

        frame.names.clear();
    }

    frame.timing = FrameProfiler::IsEnabled();
    BeginGpuTimer("Gpu.Frame");
}

void GlRenderDevice::BeginTextureReadback(TextureHandle handle, int width, int height, eTEXTURE_FORMAT format)
{
    m_readbackTarget = 1 - m_readbackTarget;
    // Half floats come back in their own size instead of the float type we upload them with
    GLenum type = format == eTextureFormat_RGBA16F ? GL_HALF_FLOAT : kTextureFormats[format].type;
    size_t size = (size_t) width * height * (format == eTextureFormat_RGBA16F ? 8 : format == eTextureFormat_RGBA32F ? 16 : 4);
    if (!m_readbackBuffers[0])
    {
        glGenBuffers(2, m_readbackBuffers);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_readbackBuffers[m_readbackTarget]);
    // Respecifying the store also unmaps the copy this buffer held two readbacks ago
    glBufferData(GL_PIXEL_PACK_BUFFER, size, nullptr, GL_STREAM_READ);
    m_readbackSize = size;
    BindTexture2D(handle);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, kTextureFormats[format].format, type, nullptr);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    m_readbackFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

const uint8_t *GlRenderDevice::PollTextureReadback()
{
    if (!m_readbackFence || glClientWaitSync((GLsync) m_readbackFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0) == GL_TIMEOUT_EXPIRED)
    {
        return nullptr;
    }

    glDeleteSync((GLsync) m_readbackFence);
    m_readbackFence = nullptr;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_readbackBuffers[m_readbackTarget]);
    const uint8_t *data = (const uint8_t *) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, m_readbackSize, GL_MAP_READ_BIT);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return data;
}

void GlRenderDevice::BeginGpuTimer(const char *name)
{
    TimerFrame &frame = m_timerFrames[m_timerFrame];
    if (!frame.timing || frame.names.size() == 32)
    {
        m_openTimers.push_back(UINT32_MAX);
        return;
    }

    uint32_t index = (uint32_t) frame.names.size();
    frame.names.push_back(name);
    m_openTimers.push_back(index);
    glQueryCounter(frame.queries[index * 2], GL_TIMESTAMP);
}

void GlRenderDevice::EndGpuTimer()
{
    uint32_t index = m_openTimers.back();
    m_openTimers.pop_back();
    if (index != UINT32_MAX)
    {
        glQueryCounter(m_timerFrames[m_timerFrame].queries[index * 2 + 1], GL_TIMESTAMP);
    }
}

void GlRenderDevice::SetVSync(bool enabled)
{
    const char *request = enabled ? "enable" : "disable";
    int result          = SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    int interval        = SDL_GL_GetSwapInterval();
    Logger::LogInfo("VSync %s request: result=%d interval=%d", request, result, interval);
    if (result != 0)
    {
        Logger::LogWarn("VSync %s failed: %s", request, SDL_GetError());
    }
}

void GlRenderDevice::GetDrawableSize(SDL_Window *window, int *width, int *height) const
{
    SDL_GL_GetDrawableSize(window, width, height);
}

void GlRenderDevice::InitImGui(SDL_Window *window)
{
    ImGui_ImplSDL2_InitForOpenGL(window, m_context);
    ImGui_ImplOpenGL3_Init("#version 460 core");
}

void GlRenderDevice::ShutdownImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
}

void GlRenderDevice::BeginImGuiFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void GlRenderDevice::RenderImGui()
{
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

const char *GlRenderDevice::GetVersion() const
{
    return (const char *) glGetString(GL_VERSION);
}

const char *GlRenderDevice::GetVendor() const
{
    return (const char *) glGetString(GL_VENDOR);
}

const char *GlRenderDevice::GetRenderer() const
{
    return (const char *) glGetString(GL_RENDERER);
}

VramStats GlRenderDevice::QueryVramStats() const
{
    VramStats stats;
    stats.available  = false;
    stats.hasTotal   = false;
    stats.totalBytes = 0;
    stats.freeBytes  = 0;
    stats.label      = L"";

    if (SDL_GL_ExtensionSupported("GL_NVX_gpu_memory_info"))
    {
        GLint dedicatedKb = 0;
        GLint totalKb     = 0;
        GLint currentKb   = 0;

        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicatedKb);
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalKb);
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &currentKb);

        if (dedicatedKb > 0 || totalKb > 0 || currentKb > 0)
        {
            stats.available  = true;
            stats.freeBytes  = currentKb > 0 ? (uint64_t) currentKb * 1024 : 0;
            stats.totalBytes = dedicatedKb > 0 ? (uint64_t) dedicatedKb * 1024 : (uint64_t) totalKb * 1024;
            stats.hasTotal   = stats.totalBytes > 0;
            stats.label      = L"NVX";
        }

        return stats;
    }

    if (SDL_GL_ExtensionSupported("GL_ATI_meminfo"))
    {
        GLint textureFreeKb[4] = {0, 0, 0, 0};
        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, textureFreeKb);

        if (textureFreeKb[0] > 0)
        {
            stats.available = true;
            stats.freeBytes = (uint64_t) textureFreeKb[0] * 1024;
            stats.label     = L"ATI";
        }
    }

    return stats;
}

void GlRenderDevice::SetDepthTest(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void GlRenderDevice::SetDepthMask(bool enabled)
{
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void GlRenderDevice::SetDepthFunc(eCOMPARE_FUNC func)
{
    glDepthFunc(kCompareFuncs[func]);
}

void GlRenderDevice::SetCull(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void GlRenderDevice::SetCullFace(eCULL_FACE face)
{
    glCullFace(kCullFaces[face]);
}

void GlRenderDevice::SetFrontFace(eFRONT_FACE winding)
{
    glFrontFace(kFrontFaces[winding]);
}

void GlRenderDevice::SetBlend(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void GlRenderDevice::SetBlendFunc(eBLEND_FACTOR src, eBLEND_FACTOR dst)
{
    glBlendFunc(kBlendFactors[src], kBlendFactors[dst]);
}

void GlRenderDevice::SetScissorTest(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_SCISSOR_TEST);
    }
    else
    {
        glDisable(GL_SCISSOR_TEST);
    }
}

void GlRenderDevice::SetScissor(int x, int y, int width, int height)
{
    glScissor(x, y, width, height);
}

void GlRenderDevice::SetColorMask(bool r, bool g, bool b, bool a)
{
    glColorMask(r, g, b, a);
}

void GlRenderDevice::SetPolygonOffsetFill(bool enabled)
{
    if (enabled)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void GlRenderDevice::SetPolygonOffset(float factor, float units)
{
    glPolygonOffset(factor, units);
}

void GlRenderDevice::SetPolygonMode(ePOLYGON_MODE mode)
{
    glPolygonMode(GL_FRONT_AND_BACK, kPolygonModes[mode]);
}

void GlRenderDevice::SetLineWidth(float width)
{
    glLineWidth(width);
}

void GlRenderDevice::SetLogicOp(eLOGIC_OP op)
{
    if (op == eLogicOp_None)
    {
        glDisable(GL_COLOR_LOGIC_OP);
        return;
    }

    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_OR_REVERSE);
}

void GlRenderDevice::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void GlRenderDevice::SetClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void GlRenderDevice::SetClearDepth(double depth)
{
    glClearDepth(depth);
}

void GlRenderDevice::ClearAll()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GlRenderDevice::ClearDepth()
{
    glClear(GL_DEPTH_BUFFER_BIT);
}

VertexArrayHandle GlRenderDevice::CreateVertexArray()
{
    VertexArrayHandle handle;
    glGenVertexArrays(1, &handle.id);
    if (handle.IsValid())
    {
        MemoryTracker::GetInstance()->GenVertexArrays(1);
    }

    return handle;
}

void GlRenderDevice::DeleteVertexArray(VertexArrayHandle handle)
{
    glDeleteVertexArrays(1, &handle.id);
    if (handle.IsValid())
    {
        MemoryTracker::GetInstance()->DeleteVertexArrays(1);
    }
}

void GlRenderDevice::BindVertexArray(VertexArrayHandle handle)
{
    glBindVertexArray(handle.id);
}

BufferHandle GlRenderDevice::CreateBuffer()
{
    BufferHandle handle;
    glGenBuffers(1, &handle.id);
    if (handle.IsValid())
    {
        MemoryTracker::GetInstance()->GenBuffers(1);
    }

    return handle;
}

void GlRenderDevice::DeleteBuffer(BufferHandle handle)
{
    glDeleteBuffers(1, &handle.id);
    if (handle.IsValid())
    {
        MemoryTracker::GetInstance()->DeleteBuffers(1);
    }
}

void GlRenderDevice::BindArrayBuffer(BufferHandle handle)
{
    glBindBuffer(GL_ARRAY_BUFFER, handle.id);
}

void GlRenderDevice::BindElementArrayBuffer(BufferHandle handle)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle.id);
}

void GlRenderDevice::UploadArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage)
{
    glBufferData(GL_ARRAY_BUFFER, size, data, kBufferUsages[usage]);
}

void GlRenderDevice::UploadElementArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage)
{
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, kBufferUsages[usage]);
}

void GlRenderDevice::UpdateArrayBuffer(uint32_t offset, uint32_t size, const void *data)
{
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void GlRenderDevice::CopyArrayBuffer(BufferHandle source, BufferHandle destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size)
{
    glBindBuffer(GL_COPY_READ_BUFFER, source.id);
    glBindBuffer(GL_COPY_WRITE_BUFFER, destination.id);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, sourceOffset, destinationOffset, size);
}

bool GlRenderDevice::HasBufferStorage()
{
    if (!s_bufferStorageChecked)
    {
        s_bufferStorageChecked = true;
        if (SDL_GL_ExtensionSupported("GL_ARB_buffer_storage"))
        {
            s_bufferStorage = (BufferStorageProc) SDL_GL_GetProcAddress("glBufferStorage");
        }

        Logger::LogInfo("Chunk buffers use %s storage", s_bufferStorage ? "immutable" : "mutable");
    }

    return s_bufferStorage != nullptr;
}

void GlRenderDevice::StorageArrayBuffer(uint32_t size)
{
    // The buffer is GPU-only and filled with glCopyBufferSubData, so without flags the driver keeps no system memory copy
    s_bufferStorage(GL_ARRAY_BUFFER, size, nullptr, 0);
}

void GlRenderDevice::SetVertexAttrib(uint32_t index, int count, eVERTEX_ATTRIB_TYPE type, bool normalized, uint32_t stride, uint32_t offset)
{
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, count, kAttribTypes[type], normalized ? GL_TRUE : GL_FALSE, stride, (const void *) (uintptr_t) offset);
}

void GlRenderDevice::SetVertexAttribDivisor(uint32_t index, uint32_t divisor)
{
    glVertexAttribDivisor(index, divisor);
}

void GlRenderDevice::DrawArrays(ePRIMITIVE_TYPE mode, int first, int count)
{
    glDrawArrays(kPrimitiveTypes[mode], first, count);
}

void GlRenderDevice::DrawElements(ePRIMITIVE_TYPE mode, int count, uint32_t offset)
{
    glDrawElements(kPrimitiveTypes[mode], count, GL_UNSIGNED_INT, (const void *) (uintptr_t) offset);
}

void GlRenderDevice::DrawMultiElementsIndirect(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, int drawCount)
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commands.id);
    s_multiDrawElementsIndirect(kPrimitiveTypes[mode], GL_UNSIGNED_INT, (const void *) (uintptr_t) offset, drawCount, 0);
}

bool GlRenderDevice::HasDrawIndirectCount()
{
    return s_multiDrawElementsIndirectCount != nullptr;
}

void GlRenderDevice::DrawMultiElementsIndirectCount(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, BufferHandle counts,
                                                    uint32_t countOffset, int maxDrawCount)
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commands.id);
    glBindBuffer(GL_PARAMETER_BUFFER, counts.id);
    s_multiDrawElementsIndirectCount(kPrimitiveTypes[mode], GL_UNSIGNED_INT, (const void *) (uintptr_t) offset, countOffset, maxDrawCount, 0);
}

bool GlRenderDevice::HasRayQuery()
{
    return false;
}

void GlRenderDevice::SetRayBoxes(uint32_t geometry, const float *boxes)
{
    (void) geometry;
    (void) boxes;
}

void GlRenderDevice::SetRayInstances(const RayInstance *instances, uint32_t count)
{
    (void) instances;
    (void) count;
}

TextureHandle GlRenderDevice::CreateTexture()
{
    TextureHandle handle;
    glGenTextures(1, &handle.id);
    if (handle.IsValid())
    {
        MemoryTracker::GetInstance()->GenTextures(1);
    }

    return handle;
}

void GlRenderDevice::DeleteTexture(TextureHandle handle)
{
    glDeleteTextures(1, &handle.id);
    if (handle.IsValid())
    {
        MemoryTracker::GetInstance()->DeleteTextures(1);
    }
}

void GlRenderDevice::BindTexture(TextureHandle handle, uint32_t slot)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, handle.id);
}

void GlRenderDevice::BindTexture2D(TextureHandle handle)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, handle.id);
}

void GlRenderDevice::UploadTexture(TextureHandle handle, int level, int width, int height, eTEXTURE_FORMAT format, const void *data)
{
    const GlTextureFormat &glFormat = kTextureFormats[format];
    BindTexture2D(handle);
    glTexImage2D(GL_TEXTURE_2D, level, glFormat.internalFormat, width, height, 0, glFormat.format, glFormat.type, data);
}

void GlRenderDevice::UploadTextureRegion(TextureHandle handle, int x, int y, int width, int height, eTEXTURE_FORMAT format, const void *data)
{
    BindTexture2D(handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, kTextureFormats[format].format, kTextureFormats[format].type, data);
}

void GlRenderDevice::SetTextureFilter(TextureHandle handle, eTEXTURE_FILTER minFilter, eTEXTURE_FILTER magFilter)
{
    BindTexture2D(handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, kTextureFilters[minFilter]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, kTextureFilters[magFilter]);
}

void GlRenderDevice::SetTextureWrap(TextureHandle handle, eTEXTURE_WRAP wrapS, eTEXTURE_WRAP wrapT)
{
    BindTexture2D(handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, kTextureWraps[wrapS]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, kTextureWraps[wrapT]);
}

void GlRenderDevice::SetTextureMaxLevel(TextureHandle handle, int maxLevel)
{
    BindTexture2D(handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);
}

void GlRenderDevice::SetTextureMaxAnisotropy(TextureHandle handle, float anisotropy)
{
    BindTexture2D(handle);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
}

float GlRenderDevice::GetMaxTextureAnisotropy() const
{
    float maxAnisotropy = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    return std::max(1.0f, maxAnisotropy);
}

uint32_t GlRenderDevice::CompileShader(uint32_t type, const char *path)
{
    std::string source = File(std::string(path) + ".glsl").ReadAllText();
    if (source.empty())
    {
        throw std::runtime_error("failed to open shader");
    }

    GLuint shader      = glCreateShader(type);
    const char *buffer = source.c_str();
    glShaderSource(shader, 1, &buffer, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        Logger::LogError("Shader compile failed %s: %s", path, log);
        throw std::runtime_error(std::string("shader compile failed ") + path + ": " + log);
    }

    return shader;
}

ProgramHandle GlRenderDevice::CreateProgram(const char *vertexPath, const char *fragmentPath, const std::vector<ShaderUniformBlock> &blocks,
                                            const std::vector<ShaderSampler> &samplers)
{
    GLuint vertex   = CompileShader(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentPath);

    ProgramHandle handle;
    handle.id = glCreateProgram();
    glAttachShader(handle.id, vertex);
    glAttachShader(handle.id, fragment);
    glLinkProgram(handle.id);

    GLint success = 0;
    glGetProgramiv(handle.id, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024] = {};
        glGetProgramInfoLog(handle.id, sizeof(log), nullptr, log);
        Logger::LogError("Program link failed %s + %s: %s", vertexPath, fragmentPath, log);
        throw std::runtime_error(std::string("program link failed ") + vertexPath + ": " + log);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    std::vector<std::vector<int>> &blockLocations = m_uniformBlockLocations[handle.id];
    blockLocations.resize(blocks.size());
    for (size_t i = 0; i < blocks.size(); i++)
    {
        blockLocations[i].resize(blocks[i].size / 16);
        for (size_t j = 0; j < blockLocations[i].size(); j++)
        {
            blockLocations[i][j] = glGetUniformLocation(handle.id, (blocks[i].name + "[" + std::to_string(j) + "]").c_str());
        }
    }

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glUseProgram(handle.id);
    for (size_t i = 0; i < samplers.size(); i++)
    {
        glUniform1i(glGetUniformLocation(handle.id, samplers[i].name.c_str()), samplers[i].binding);
    }

    glUseProgram(previousProgram);
    Logger::LogInfo("Created program %u from %s + %s (%zu blocks, %zu samplers)", handle.id, vertexPath, fragmentPath, blocks.size(),
                    samplers.size());
    return handle;
}

void GlRenderDevice::DeleteProgram(ProgramHandle handle)
{
    m_uniformBlockLocations.erase(handle.id);
    m_computeSamplers.erase(handle.id);
    glDeleteProgram(handle.id);
}

void GlRenderDevice::UseProgram(ProgramHandle handle)
{
    glUseProgram(handle.id);
}

void GlRenderDevice::UploadUniformBlock(ProgramHandle handle, int block, int firstVec4, int vec4Count, const float *data)
{
    // The driver strips unused trailing elements and ignores the part of a count past the last active one
    int location = m_uniformBlockLocations[handle.id][block][firstVec4];
    if (location >= 0)
    {
        glUniform4fv(location, vec4Count, data);
    }
}

ProgramHandle GlRenderDevice::CreateComputeProgram(const char *path, const ShaderUniformBlock &block, int bufferCount,
                                                   const std::vector<ShaderSampler> &samplers)
{
    (void) bufferCount;
    GLuint shader = CompileShader(GL_COMPUTE_SHADER, path);

    ProgramHandle handle;
    handle.id = glCreateProgram();
    glAttachShader(handle.id, shader);
    glLinkProgram(handle.id);

    GLint success = 0;
    glGetProgramiv(handle.id, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024] = {};
        glGetProgramInfoLog(handle.id, sizeof(log), nullptr, log);
        Logger::LogError("Program link failed %s: %s", path, log);
        throw std::runtime_error(std::string("program link failed ") + path + ": " + log);
    }

    glDeleteShader(shader);

    std::vector<int> &locations = m_uniformBlockLocations[handle.id].emplace_back();
    locations.resize(block.size / 16);
    for (size_t i = 0; i < locations.size(); i++)
    {
        locations[i] = glGetUniformLocation(handle.id, (block.name + "[" + std::to_string(i) + "]").c_str());
    }

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glUseProgram(handle.id);
    std::vector<uint32_t> &units = m_computeSamplers[handle.id];
    for (size_t i = 0; i < samplers.size(); i++)
    {
        glUniform1i(glGetUniformLocation(handle.id, samplers[i].name.c_str()), samplers[i].binding);
        units.push_back(samplers[i].binding);
    }

    glUseProgram(previousProgram);
    Logger::LogInfo("Created compute program %u from %s", handle.id, path);
    return handle;
}

void GlRenderDevice::ClearBuffer(BufferHandle handle)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle.id);
    s_clearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
}

void GlRenderDevice::DispatchCompute(ProgramHandle handle, const float *uniforms, const BufferHandle *buffers, int bufferCount,
                                     const TextureHandle *textures, uint32_t groupCount)
{
    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glUseProgram(handle.id);
    std::vector<int> &locations = m_uniformBlockLocations[handle.id][0];
    glUniform4fv(locations[0], (GLsizei) locations.size(), uniforms);
    for (int i = 0; i < bufferCount; i++)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i + 1, buffers[i].id);
    }

    GLint previousUnit = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousUnit);
    const std::vector<uint32_t> &units = m_computeSamplers[handle.id];
    std::vector<GLint> previousTextures(units.size());
    for (size_t i = 0; i < units.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + units[i]);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextures[i]);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    s_dispatchCompute(groupCount, 1, 1);
    s_memoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    for (size_t i = 0; i < units.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + units[i]);
        glBindTexture(GL_TEXTURE_2D, previousTextures[i]);
    }

    glActiveTexture(previousUnit);
    glUseProgram(previousProgram);
}

FramebufferHandle GlRenderDevice::CreateFramebuffer(TextureHandle color, TextureHandle depth)
{
    FramebufferHandle handle;
    glGenFramebuffers(1, &handle.id);
    glBindFramebuffer(GL_FRAMEBUFFER, handle.id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color.id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth.id, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &handle.id);
        Logger::LogError("Framebuffer incomplete: status=0x%X color=%u depth=%u", status, color.id, depth.id);
        throw std::runtime_error("framebuffer incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Logger::LogInfo("Created framebuffer %u color=%u depth=%u", handle.id, color.id, depth.id);
    return handle;
}

void GlRenderDevice::DeleteFramebuffer(FramebufferHandle handle)
{
    Logger::LogInfo("Deleted framebuffer %u", handle.id);
    glDeleteFramebuffers(1, &handle.id);
}

void GlRenderDevice::BindFramebuffer(FramebufferHandle handle)
{
    glBindFramebuffer(GL_FRAMEBUFFER, handle.id);
}

void GlRenderDevice::BlitFramebufferDepth(FramebufferHandle source, int sourceWidth, int sourceHeight, FramebufferHandle destination,
                                          int destinationWidth, int destinationHeight)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.id);
    glBlitFramebuffer(0, 0, sourceWidth, sourceHeight, 0, 0, destinationWidth, destinationHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, destination.id);
}

void GlRenderDevice::ReadPixels(int x, int y, int width, int height, uint8_t *pixels)
{
    Logger::LogInfo("Reading back %dx%d pixels at %d,%d, this stalls until the GPU finishes", width, height, x, y);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

#endif
