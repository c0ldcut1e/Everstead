#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../../rendering/RenderTypes.h"
#include "../../rendering/Texture.h"
#include "../../rendering/TextureAtlas.h"

class ChunkMesh
{
public:
    static constexpr uint32_t kVertexLightScale = 4;
    static constexpr float kUvScale             = 1024.0f;
    // Positions are integer steps per block so coplanar edges land on identical coordinates
    static constexpr float kPositionStepsXZ = 512.0f;
    static constexpr float kPositionStepsY  = 128.0f;

    // This stays small because the NVIDIA Windows driver charges several times the VBO size to process commit
    // Every attribute starts on a 4-byte boundary since AMD drivers fall back to a software vertex fetch otherwise
    // The atlas rect is normalized to the full uint16 range and position[3] packs the octahedral normal as two biased bytes
    // The tint alpha holds the tint mode in its high bit and the shade in the rest
    struct Vertex
    {
        uint16_t atlasRect[4];
        int16_t uv[2];
        uint8_t tint[4];
        uint8_t rawLight[4];
        uint16_t position[4];
    };

    static_assert(sizeof(Vertex) == 28, "ChunkMesh::Vertex must stay 28 bytes");

    struct PackedData
    {
        Texture *texture;
        bool translucent;
        bool sortQuads;
        uint32_t vertexCount;
        std::vector<Vertex> vertices;
        std::vector<TextureAtlas::Region> atlasRegions;
    };

    static PackedData PackData(const float *vertices, uint32_t vertexCount, std::vector<float> &&atlasRects, std::vector<uint32_t> &&rawLights,
                               std::vector<float> &&shades, std::vector<uint32_t> &&tints, Texture *texture, bool translucent);
    static void AppendUniqueRegion(std::vector<TextureAtlas::Region> *regions, const TextureAtlas::Region &region);

    ChunkMesh(Texture *texture, bool translucent, bool sortQuads);
    ~ChunkMesh();

    uint32_t GetCapacity() const;
    void Allocate(uint32_t capacity);
    void CopyVertices(const ChunkMesh &source, int sourceFirst, int destinationFirst, int count);
    void SetVertexCount(uint32_t vertexCount);
    const std::vector<TextureAtlas::Region> &GetAtlasRegions() const;
    void AddAtlasRegions(const std::vector<TextureAtlas::Region> &regions);
    void UpdateVertices(int firstVertex, const std::vector<Vertex> &vertices);
    uint32_t GetVertexCount() const;
    uint32_t GetPage() const;
    uint32_t GetFirstVertex() const;
    static uint64_t GetGpuBytes();

    bool IsTranslucent() const;
    bool SortsQuads() const;
    Texture *GetTexture() const;
    void AppendAtlasRegions(std::vector<TextureAtlas::Region> *regions) const;
    size_t GetCpuBytes() const;

    static void BindQuadIndices(uint32_t quadCount);

private:
    void SetCapacity(uint32_t capacity);

    static std::atomic<uint64_t> s_gpuBytes;

    static BufferHandle s_indexBuffer;
    static uint32_t s_indexQuadCapacity;
    static uint16_t PackPositionComponent(float value, float maxValue, float steps);
    static uint8_t PackNormalizedByte(float value);
    static uint8_t PackBiasedNormalByte(double value);
    static uint8_t PackVertexLightChannel(uint32_t vertexLight, int channel);

    Texture *m_texture;
    uint32_t m_page;
    uint32_t m_first;
    uint32_t m_vertexCount;
    uint32_t m_capacity;
    bool m_translucent;
    bool m_sortQuads;
    std::vector<TextureAtlas::Region> m_atlasRegions;
};
