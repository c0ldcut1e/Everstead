#pragma once

#include <cstdint>
#include <vector>

#include "RenderTypes.h"

#include <SDL2/SDL.h>

class RenderDevice
{
public:
    RenderDevice();
    virtual ~RenderDevice();

    static RenderDevice *GetInstance()
    {
        return s_instance;
    }

    virtual eRENDER_BACKEND GetBackend() const                                      = 0;
    virtual uint32_t GetWindowFlags() const                                         = 0;
    virtual void CreateContext(SDL_Window *window)                                  = 0;
    virtual void Present(SDL_Window *window)                                        = 0;
    virtual void SetVSync(bool enabled)                                             = 0;
    virtual void GetDrawableSize(SDL_Window *window, int *width, int *height) const = 0;

    virtual void InitImGui(SDL_Window *window) = 0;
    virtual void ShutdownImGui()               = 0;
    virtual void BeginImGuiFrame()             = 0;
    virtual void RenderImGui()                 = 0;

    virtual const char *GetVersion() const   = 0;
    virtual const char *GetVendor() const    = 0;
    virtual const char *GetRenderer() const  = 0;
    virtual VramStats QueryVramStats() const = 0;

    virtual void SetDepthTest(bool enabled)                         = 0;
    virtual void SetDepthMask(bool enabled)                         = 0;
    virtual void SetDepthFunc(eCOMPARE_FUNC func)                   = 0;
    virtual void SetCull(bool enabled)                              = 0;
    virtual void SetCullFace(eCULL_FACE face)                       = 0;
    virtual void SetFrontFace(eFRONT_FACE winding)                  = 0;
    virtual void SetBlend(bool enabled)                             = 0;
    virtual void SetBlendFunc(eBLEND_FACTOR src, eBLEND_FACTOR dst) = 0;
    virtual void SetScissorTest(bool enabled)                       = 0;
    virtual void SetScissor(int x, int y, int width, int height)    = 0;
    virtual void SetColorMask(bool r, bool g, bool b, bool a)       = 0;
    virtual void SetPolygonOffsetFill(bool enabled)                 = 0;
    virtual void SetPolygonOffset(float factor, float units)        = 0;
    virtual void SetPolygonMode(ePOLYGON_MODE mode)                 = 0;
    virtual void SetLineWidth(float width)                          = 0;
    virtual void SetLogicOp(eLOGIC_OP op)                           = 0;

    virtual void SetViewport(int x, int y, int width, int height)  = 0;
    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void SetClearDepth(double depth)                       = 0;
    virtual void ClearAll()                                        = 0;
    virtual void ClearDepth()                                      = 0;

    virtual VertexArrayHandle CreateVertexArray()            = 0;
    virtual void DeleteVertexArray(VertexArrayHandle handle) = 0;
    virtual void BindVertexArray(VertexArrayHandle handle)   = 0;
    virtual BufferHandle CreateBuffer()                      = 0;
    virtual void DeleteBuffer(BufferHandle handle)           = 0;
    virtual void BindArrayBuffer(BufferHandle handle)        = 0;
    virtual void BindElementArrayBuffer(BufferHandle handle) = 0;

    virtual void UploadArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage)                                                          = 0;
    virtual void UploadElementArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage)                                                   = 0;
    virtual void UpdateArrayBuffer(uint32_t offset, uint32_t size, const void *data)                                                              = 0;
    virtual void CopyArrayBuffer(BufferHandle source, BufferHandle destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size) = 0;
    virtual bool HasBufferStorage()                                                                                                               = 0;
    virtual void StorageArrayBuffer(uint32_t size)                                                                                                = 0;
    virtual void SetVertexAttrib(uint32_t index, int count, eVERTEX_ATTRIB_TYPE type, bool normalized, uint32_t stride, uint32_t offset)          = 0;
    virtual void SetVertexAttribDivisor(uint32_t index, uint32_t divisor)                                                                         = 0;

    virtual void DrawArrays(ePRIMITIVE_TYPE mode, int first, int count)                                                 = 0;
    virtual void DrawElements(ePRIMITIVE_TYPE mode, int count, uint32_t offset)                                         = 0;
    virtual void DrawMultiElementsIndirect(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, int drawCount) = 0;
    virtual bool HasDrawIndirectCount()                                                                                 = 0;
    virtual void DrawMultiElementsIndirectCount(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, BufferHandle counts,
                                                uint32_t countOffset, int maxDrawCount)                                 = 0;
    virtual bool HasRayQuery()                                                                                          = 0;
    virtual void SetRayBoxes(uint32_t geometry, const float *boxes)                                                     = 0;
    virtual void SetRayInstances(const RayInstance *instances, uint32_t count)                                          = 0;

    virtual TextureHandle CreateTexture()                                                                                                 = 0;
    virtual void DeleteTexture(TextureHandle handle)                                                                                      = 0;
    virtual void BindTexture(TextureHandle handle, uint32_t slot)                                                                         = 0;
    virtual void UploadTexture(TextureHandle handle, int level, int width, int height, eTEXTURE_FORMAT format, const void *data)          = 0;
    virtual void UploadTextureRegion(TextureHandle handle, int x, int y, int width, int height, eTEXTURE_FORMAT format, const void *data) = 0;
    virtual void SetTextureFilter(TextureHandle handle, eTEXTURE_FILTER minFilter, eTEXTURE_FILTER magFilter)                             = 0;
    virtual void SetTextureWrap(TextureHandle handle, eTEXTURE_WRAP wrapS, eTEXTURE_WRAP wrapT)                                           = 0;
    virtual void SetTextureMaxLevel(TextureHandle handle, int maxLevel)                                                                   = 0;
    virtual void SetTextureMaxAnisotropy(TextureHandle handle, float anisotropy)                                                          = 0;
    virtual float GetMaxTextureAnisotropy() const                                                                                         = 0;

    virtual ProgramHandle CreateProgram(const char *vertexPath, const char *fragmentPath, const std::vector<ShaderUniformBlock> &blocks,
                                        const std::vector<ShaderSampler> &samplers)                                   = 0;
    virtual void DeleteProgram(ProgramHandle handle)                                                                  = 0;
    virtual void UseProgram(ProgramHandle handle)                                                                     = 0;
    virtual void UploadUniformBlock(ProgramHandle handle, int block, int firstVec4, int vec4Count, const float *data) = 0;
    // Storage buffers bind from 1 in order after the uniform block at 0, textures follow the sampler order, and the graphics program stays bound
    virtual ProgramHandle CreateComputeProgram(const char *path, const ShaderUniformBlock &block, int bufferCount,
                                               const std::vector<ShaderSampler> &samplers) = 0;
    virtual void ClearBuffer(BufferHandle handle)                                          = 0;
    virtual void DispatchCompute(ProgramHandle handle, const float *uniforms, const BufferHandle *buffers, int bufferCount,
                                 const TextureHandle *textures, uint32_t groupCount)       = 0;

    virtual FramebufferHandle CreateFramebuffer(TextureHandle color, TextureHandle depth) = 0;
    virtual void DeleteFramebuffer(FramebufferHandle handle)                              = 0;
    virtual void BindFramebuffer(FramebufferHandle handle)                                = 0;
    virtual void BlitFramebufferDepth(FramebufferHandle source, int sourceWidth, int sourceHeight, FramebufferHandle destination,
                                      int destinationWidth, int destinationHeight)        = 0;
    virtual void ReadPixels(int x, int y, int width, int height, uint8_t *pixels)         = 0;
    // Readbacks alternate between two host buffers, so the data Poll returned stays valid until the readback after next begins
    virtual void BeginTextureReadback(TextureHandle handle, int width, int height, eTEXTURE_FORMAT format) = 0;
    virtual const uint8_t *PollTextureReadback()                                                           = 0;

    // Timers only record while the frame profiler runs and are read back frames later, so they never wait on the GPU
    virtual void BeginGpuTimer(const char *name) = 0;
    virtual void EndGpuTimer()                   = 0;

private:
    static inline RenderDevice *s_instance = nullptr;
};
