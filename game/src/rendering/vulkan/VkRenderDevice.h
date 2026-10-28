#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../RenderDevice.h"
#include "VkAllocator.h"
#include "VkContext.h"
#include "VkDeletionQueue.h"
#include "VkDescriptorAllocator.h"
#include "VkPipelineStore.h"
#include "VkRayScene.h"
#include "VkResources.h"
#include "VkSamplerStore.h"
#include "VkStreamRing.h"
#include "VkSwapchain.h"

#include <SDL2/SDL.h>

class VkRenderDevice : public RenderDevice
{
public:
    VkRenderDevice();
    ~VkRenderDevice() override;

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

    const VkContext &GetContext() const;
    VkAllocator &GetAllocator();
    VkDeletionQueue &GetDeletionQueue();
    VkBufferData &UseBuffer(BufferHandle handle);
    VkTextureData &UseTexture(TextureHandle handle);
    VkCommandBuffer BeginExternalCommands(VkPipelineStageFlags dstStages, VkAccessFlags dstAccess);
    void EndExternalCommands(VkPipelineStageFlags srcStages, VkAccessFlags srcAccess);

private:
    struct Frame
    {
        VkCommandPool pool;
        VkCommandBuffer cmd;
        VkCommandBuffer uploadCmd;
        bool uploading;
        uint64_t submit;
        VkSemaphore imageAvailable;
        VkStreamRing ring;
        VkDescriptorAllocator descriptors;
        VkDeletionQueue deletions;
        VkQueryPool timers;
        std::vector<const char *> timerNames;
        bool timing;
    };

    static constexpr uint32_t kMaxGpuTimers = 32;

    void BeginFrame();
    void BeginCommands();
    void Submit(VkSemaphore wait, VkSemaphore signal);
    void WaitForSubmit(uint64_t submit);
    void RecreateSwapchain(int width, int height);
    void ResizeBackbuffer(int width, int height);

    void BeginRenderPass(VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD, VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD);
    void EndRenderPass();
    void MainBarrier(VkPipelineStageFlags dstStages, VkAccessFlags dstAccess);
    void TransitionImage(VkCommandBuffer cmd, VkTextureData *texture, VkImageLayout layout, VkPipelineStageFlags2KHR stages,
                         VkAccessFlags2KHR access);
    void BeginUploads();
    VkCommandBuffer GetTransferCommands(VkUsage *destination, VkUsage *source);
    void PrepareDraw(ePRIMITIVE_TYPE mode);
    void BindIndexBuffer(VkBufferData &buffer, VkDeviceSize offset);

    void UploadBuffer(VkBufferData *buffer, const void *data, uint32_t size, eBUFFER_USAGE usage);
    void ReleaseBuffer(VkBufferData *buffer);
    void StageBuffer(VkBufferData *buffer, VkDeviceSize offset, const void *data, VkDeviceSize size);

    void CreateImage(VkTextureData *texture, int width, int height, eTEXTURE_FORMAT format, uint32_t levels);
    void ReleaseImage(VkTextureData *texture);
    void StageImage(VkTextureData *texture, int level, int x, int y, int width, int height, const void *data);
    uint32_t CreateTextureData();

    VkFramebufferData &GetFramebuffer();
    VkRect2D GetScissorRect(const VkTextureData &color) const;

    VkContext m_context;
    VkAllocator m_allocator;
    VkSwapchain m_swapchain;
    int m_swapchainWidth;
    int m_swapchainHeight;
    VkPipelineStore m_pipelines;
    VkSamplerStore m_samplers;
    VkRayScene m_rayScene;
    Frame m_frames[2];
    uint32_t m_frameIndex;
    uint64_t m_frameNumber;
    std::vector<uint32_t> m_openTimers;
    uint64_t m_submitNumber;
    VkSemaphore m_timeline;
    uint64_t m_uploadEpoch;
    VkPipelineStageFlags m_pendingStages;
    VkAccessFlags m_pendingAccess;

    std::unordered_map<uint32_t, VkBufferData> m_buffers;
    std::unordered_map<uint32_t, VkTextureData> m_textures;
    std::unordered_map<uint32_t, VkVertexArrayData> m_vertexArrays;
    std::unordered_map<uint32_t, VkFramebufferData> m_framebuffers;
    std::unordered_map<uint32_t, VkProgramData> m_programs;
    std::unordered_map<uint32_t, VkComputeProgramData> m_computePrograms;
    uint32_t m_nextId;

    VkBufferData m_dummyBuffer;
    VkBufferData m_readbacks[2];
    int m_readbackTarget;
    bool m_readbackPending;
    uint64_t m_readbackSubmit;
    uint32_t m_backbufferColor;
    uint32_t m_backbufferDepth;

    VkPipelineKey m_state;
    uint32_t m_arrayBuffer;
    uint32_t m_vertexArray;
    uint32_t m_program;
    uint32_t m_framebuffer;
    uint32_t m_textureBindings[16];

    bool m_inRenderPass;
    bool m_dynamicDirty;
    bool m_uniformsBound;
    bool m_samplersDirty;
    VkPipeline m_boundPipeline;

    VkViewport m_viewport;
    VkRect2D m_scissor;
    bool m_scissorTest;
    float m_clearColor[4];
    float m_clearDepth;
    float m_depthBiasFactor;
    float m_depthBiasUnits;
    float m_lineWidth;

    bool m_vsync;
    bool m_imguiPending;
    std::string m_version;
};
