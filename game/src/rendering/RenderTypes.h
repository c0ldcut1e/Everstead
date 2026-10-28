#pragma once

#include <cstdint>
#include <string>

enum eRENDER_BACKEND : uint8_t
{
    eRenderBackend_OpenGL = 0,
    eRenderBackend_Vulkan
};

enum eRENDER_HANDLE : uint8_t
{
    eRenderHandle_Texture = 0,
    eRenderHandle_Buffer,
    eRenderHandle_VertexArray,
    eRenderHandle_Framebuffer,
    eRenderHandle_Program
};

template<eRENDER_HANDLE T>
struct RenderHandle
{
    uint32_t id = 0;

    bool IsValid() const
    {
        return id != 0;
    }

    bool operator==(const RenderHandle &other) const = default;
};

typedef RenderHandle<eRenderHandle_Texture> TextureHandle;
typedef RenderHandle<eRenderHandle_Buffer> BufferHandle;
typedef RenderHandle<eRenderHandle_VertexArray> VertexArrayHandle;
typedef RenderHandle<eRenderHandle_Framebuffer> FramebufferHandle;
typedef RenderHandle<eRenderHandle_Program> ProgramHandle;

struct ShaderUniformBlock
{
    std::string name;
    uint32_t size;
};

struct ShaderSampler
{
    std::string name;
    uint32_t binding;
};

enum ePRIMITIVE_TYPE : uint8_t
{
    ePrimitiveType_Triangles = 0,
    ePrimitiveType_Lines,
    ePrimitiveType_LineLoop,
    ePrimitiveType_TriangleFan
};

enum eCOMPARE_FUNC : uint8_t
{
    eCompareFunc_Less = 0,
    eCompareFunc_LessEqual,
    eCompareFunc_Equal,
    eCompareFunc_Greater
};

enum eBLEND_FACTOR : uint8_t
{
    eBlendFactor_Zero = 0,
    eBlendFactor_One,
    eBlendFactor_SrcAlpha,
    eBlendFactor_OneMinusSrcAlpha,
    eBlendFactor_SrcColor,
    eBlendFactor_OneMinusSrcColor,
    eBlendFactor_DstColor
};

enum eCULL_FACE : uint8_t
{
    eCullFace_Back = 0
};

enum eFRONT_FACE : uint8_t
{
    eFrontFace_Ccw = 0
};

enum ePOLYGON_MODE : uint8_t
{
    ePolygonMode_Fill = 0,
    ePolygonMode_Line
};

enum eTEXTURE_FILTER : uint8_t
{
    eTextureFilter_Nearest = 0,
    eTextureFilter_Linear,
    eTextureFilter_NearestMipmapNearest
};

enum eTEXTURE_WRAP : uint8_t
{
    eTextureWrap_Repeat = 0,
    eTextureWrap_ClampToEdge
};

enum eTEXTURE_FORMAT : uint8_t
{
    eTextureFormat_RGBA8 = 0,
    eTextureFormat_RGBA16F,
    eTextureFormat_RGBA32F,
    eTextureFormat_Depth32F,
    eTextureFormat_R32UI
};

enum eVERTEX_ATTRIB_TYPE : uint8_t
{
    eVertexAttribType_Float = 0,
    eVertexAttribType_UnsignedShort,
    eVertexAttribType_Short,
    eVertexAttribType_UnsignedByte
};

enum eBUFFER_USAGE : uint8_t
{
    eBufferUsage_Static = 0,
    eBufferUsage_Stream
};

enum eLOGIC_OP : uint8_t
{
    eLogicOp_None = 0,
    eLogicOp_OrReverse
};

struct VramStats
{
    bool available;
    bool hasTotal;
    uint64_t totalBytes;
    uint64_t freeBytes;
    std::wstring label;
};

// GL and Vulkan read this layout straight from the indirect buffer
struct DrawIndirectCommand
{
    uint32_t count;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int baseVertex;
    uint32_t baseInstance;
};
static_assert(sizeof(DrawIndirectCommand) == 20, "DrawIndirectCommand must match the API layout");

constexpr uint32_t kRayGeometries = 4096;
constexpr uint32_t kRayBoxes      = 64;

struct RayInstance
{
    uint32_t geometry;
    uint32_t customIndex;
    float x;
    float y;
    float z;
};
