#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../RenderDevice.h"

#include <SDL2/SDL.h>

class GlRenderDevice : public RenderDevice
{
public:
    GlRenderDevice();
    ~GlRenderDevice() override;

    eRENDER_BACKEND GetBackend() const override;
    uint32_t GetWindowFlags() const override;
    void CreateContext(SDL_Window *window) override;
    void Present(SDL_Window *window) override;
    void SetVSync(bool enabled) override;
    void GetDrawableSize(SDL_Window *window, int *width, int *height) const override;

    void InitImGui(SDL_Window *window) override;
    void ShutdownImGui() override;
    void BeginImGuiFrame() override;
    void RenderImGui() override;

    const char *GetVersion() const override;
    const char *GetVendor() const override;
    const char *GetRenderer() const override;
    VramStats QueryVramStats() const override;

    void SetDepthTest(bool enabled) override;
    void SetDepthMask(bool enabled) override;
    void SetDepthFunc(eCOMPARE_FUNC func) override;
    void SetCull(bool enabled) override;
    void SetCullFace(eCULL_FACE face) override;
    void SetFrontFace(eFRONT_FACE winding) override;
    void SetBlend(bool enabled) override;
    void SetBlendFunc(eBLEND_FACTOR src, eBLEND_FACTOR dst) override;
    void SetScissorTest(bool enabled) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetColorMask(bool r, bool g, bool b, bool a) override;
    void SetPolygonOffsetFill(bool enabled) override;
    void SetPolygonOffset(float factor, float units) override;
    void SetPolygonMode(ePOLYGON_MODE mode) override;
    void SetLineWidth(float width) override;
    void SetLogicOp(eLOGIC_OP op) override;

    void SetViewport(int x, int y, int width, int height) override;
    void SetClearColor(float r, float g, float b, float a) override;
    void SetClearDepth(double depth) override;
    void ClearAll() override;
    void ClearDepth() override;

    VertexArrayHandle CreateVertexArray() override;
    void DeleteVertexArray(VertexArrayHandle handle) override;
    void BindVertexArray(VertexArrayHandle handle) override;
    BufferHandle CreateBuffer() override;
    void DeleteBuffer(BufferHandle handle) override;
    void BindArrayBuffer(BufferHandle handle) override;
    void BindElementArrayBuffer(BufferHandle handle) override;

    void UploadArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage) override;
    void UploadElementArrayBuffer(const void *data, uint32_t size, eBUFFER_USAGE usage) override;
    void UpdateArrayBuffer(uint32_t offset, uint32_t size, const void *data) override;
    void CopyArrayBuffer(BufferHandle source, BufferHandle destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size) override;
    bool HasBufferStorage() override;
    void StorageArrayBuffer(uint32_t size) override;
    void SetVertexAttrib(uint32_t index, int count, eVERTEX_ATTRIB_TYPE type, bool normalized, uint32_t stride, uint32_t offset) override;
    void SetVertexAttribDivisor(uint32_t index, uint32_t divisor) override;

    void DrawArrays(ePRIMITIVE_TYPE mode, int first, int count) override;
    void DrawElements(ePRIMITIVE_TYPE mode, int count, uint32_t offset) override;
    void DrawMultiElementsIndirect(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, int drawCount) override;
    bool HasDrawIndirectCount() override;
    void DrawMultiElementsIndirectCount(ePRIMITIVE_TYPE mode, BufferHandle commands, uint32_t offset, BufferHandle counts, uint32_t countOffset,
                                        int maxDrawCount) override;
    bool HasRayQuery() override;
    void SetRayBoxes(uint32_t geometry, const float *boxes) override;
    void SetRayInstances(const RayInstance *instances, uint32_t count) override;

    TextureHandle CreateTexture() override;
    void DeleteTexture(TextureHandle handle) override;
    void BindTexture(TextureHandle handle, uint32_t slot) override;
    void UploadTexture(TextureHandle handle, int level, int width, int height, eTEXTURE_FORMAT format, const void *data) override;
    void UploadTextureRegion(TextureHandle handle, int x, int y, int width, int height, eTEXTURE_FORMAT format, const void *data) override;
    void SetTextureFilter(TextureHandle handle, eTEXTURE_FILTER minFilter, eTEXTURE_FILTER magFilter) override;
    void SetTextureWrap(TextureHandle handle, eTEXTURE_WRAP wrapS, eTEXTURE_WRAP wrapT) override;
    void SetTextureMaxLevel(TextureHandle handle, int maxLevel) override;
    void SetTextureMaxAnisotropy(TextureHandle handle, float anisotropy) override;
    float GetMaxTextureAnisotropy() const override;

    ProgramHandle CreateProgram(const char *vertexPath, const char *fragmentPath, const std::vector<ShaderUniformBlock> &blocks,
                                const std::vector<ShaderSampler> &samplers) override;
    void DeleteProgram(ProgramHandle handle) override;
    void UseProgram(ProgramHandle handle) override;
    void UploadUniformBlock(ProgramHandle handle, int block, int firstVec4, int vec4Count, const float *data) override;
    ProgramHandle CreateComputeProgram(const char *path, const ShaderUniformBlock &block, int bufferCount,
                                       const std::vector<ShaderSampler> &samplers) override;
    void ClearBuffer(BufferHandle handle) override;
    void DispatchCompute(ProgramHandle handle, const float *uniforms, const BufferHandle *buffers, int bufferCount, const TextureHandle *textures,
                         uint32_t groupCount) override;

    FramebufferHandle CreateFramebuffer(TextureHandle color, TextureHandle depth) override;
    void DeleteFramebuffer(FramebufferHandle handle) override;
    void BindFramebuffer(FramebufferHandle handle) override;
    void BlitFramebufferDepth(FramebufferHandle source, int sourceWidth, int sourceHeight, FramebufferHandle destination, int destinationWidth,
                              int destinationHeight) override;
    void ReadPixels(int x, int y, int width, int height, uint8_t *pixels) override;
    void BeginTextureReadback(TextureHandle handle, int width, int height, eTEXTURE_FORMAT format) override;
    const uint8_t *PollTextureReadback() override;

    void BeginGpuTimer(const char *name) override;
    void EndGpuTimer() override;

private:
    struct TimerFrame
    {
        uint32_t queries[64];
        std::vector<const char *> names;
        bool timing;
    };

    void BindTexture2D(TextureHandle handle);
    void BeginTimerFrame();
    uint32_t CompileShader(uint32_t type, const char *path);

    // Each block is a flattened vec4 array in GL, so we keep the location of every element per program
    std::unordered_map<uint32_t, std::vector<std::vector<int>>> m_uniformBlockLocations;
    std::unordered_map<uint32_t, std::vector<uint32_t>> m_computeSamplers;
    SDL_GLContext m_context;
    // Results are read four frames late so the query is done before we ask for it
    TimerFrame m_timerFrames[4];
    size_t m_timerFrame;
    std::vector<uint32_t> m_openTimers;
    uint32_t m_readbackBuffers[2];
    size_t m_readbackSize;
    int m_readbackTarget;
    void *m_readbackFence;
};
