#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesh.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include "../../rendering/BlockAtlas.h"
#include "../../rendering/RenderDevice.h"
#include "../block/BlockRegistry.h"
#include "Chunk.h"
#include "ChunkMesher.h"
#include "ChunkVertexArena.h"

#include <glm/geometric.hpp>

BufferHandle ChunkMesh::s_indexBuffer;
uint32_t ChunkMesh::s_indexQuadCapacity = 0;
std::atomic<uint64_t> ChunkMesh::s_gpuBytes{0};

uint16_t ChunkMesh::PackPositionComponent(float value, float maxValue, float steps)
{
    return (uint16_t) lroundf(std::clamp(value, 0.0f, maxValue) * steps);
}

uint8_t ChunkMesh::PackNormalizedByte(float value)
{
    float normalized = std::clamp(value, 0.0f, 1.0f);
    return (uint8_t) (normalized * 255.0f + 0.5f);
}

uint8_t ChunkMesh::PackBiasedNormalByte(double value)
{
    double normalized = std::clamp(value, -1.0, 1.0);
    return (uint8_t) (std::lround(normalized * 127.0) + 127);
}

uint8_t ChunkMesh::PackVertexLightChannel(uint32_t vertexLight, int channel)
{
    uint32_t maxValue = 15 * kVertexLightScale;
    uint32_t value    = std::min((vertexLight >> (channel * 8)) & 0xFF, maxValue);
    return (uint8_t) ((value * 255 + maxValue / 2) / maxValue);
}

ChunkMesh::ChunkMesh(Texture *texture, bool translucent, bool sortQuads)
{
    m_texture     = texture;
    m_vertexCount = 0;
    m_capacity    = 0;
    m_translucent = translucent;
    m_sortQuads   = sortQuads;
    m_page        = 0;
    m_first       = 0;
}

ChunkMesh::~ChunkMesh()
{
    if (m_capacity > 0)
    {
        ChunkVertexArena::GetInstance()->Free(m_page, m_first, m_capacity);
    }

    SetCapacity(0);
}

ChunkMesh::PackedData ChunkMesh::PackData(const float *vertices, uint32_t vertexCount, std::vector<float> &&atlasRects,
                                          std::vector<uint32_t> &&rawLights, std::vector<float> &&shades, std::vector<uint32_t> &&tints,
                                          Texture *texture, bool translucent)
{
    PackedData data;
    data.texture     = texture;
    data.translucent = translucent;
    data.sortQuads   = false;
    data.vertexCount = vertexCount / 8;
    data.vertices.resize((size_t) data.vertexCount);

    TextureAtlas *atlas = nullptr;
    if (texture == BlockAtlas::GetInstance()->GetBlockAtlas()->GetTexture())
    {
        atlas = BlockAtlas::GetInstance()->GetBlockAtlas();
    }
    else if (texture == BlockAtlas::GetInstance()->GetFlowAtlas()->GetTexture())
    {
        atlas = BlockAtlas::GetInstance()->GetFlowAtlas();
    }

    for (uint32_t i = 0; i < data.vertexCount; i++)
    {
        const float *source = vertices + (size_t) i * 8;
        const float *rect   = atlasRects.data() + (size_t) i * 4;
        Vertex &vertex      = data.vertices[i];
        vertex.position[0]  = PackPositionComponent(source[0], (float) Chunk::s_sizeX, kPositionStepsXZ);
        vertex.position[1]  = PackPositionComponent(source[1], (float) Chunk::s_sizeY, kPositionStepsY);
        vertex.position[2]  = PackPositionComponent(source[2], (float) Chunk::s_sizeZ, kPositionStepsXZ);
        vertex.uv[0]        = (int16_t) lroundf(source[3] * kUvScale);
        vertex.uv[1]        = (int16_t) lroundf(source[4] * kUvScale);
        vertex.tint[0]      = (uint8_t) ((tints[i] >> 16) & 0xFF);
        vertex.tint[1]      = (uint8_t) ((tints[i] >> 8) & 0xFF);
        vertex.tint[2]      = (uint8_t) (tints[i] & 0xFF);
        // High bit is the masked tint mode, the next bit flags snowy leaves and the low 6 bits carry shade
        uint32_t tintMode = (tints[i] >> 24) & 0xFF;
        uint8_t modeBits  = tintMode == ChunkMesher::s_tintModeMasked ? 0x80 : (tintMode == ChunkMesher::s_tintModeSnowyLeaf ? 0x40 : 0);
        vertex.tint[3]    = (uint8_t) (modeBits | (PackNormalizedByte(shades[i]) * 63 + 127) / 255);
        for (int corner = 0; corner < 4; corner++)
        {
            // We round the min corner up and the max corner down so the rect never reaches into a neighbouring atlas cell
            float scaled             = std::clamp(rect[corner], 0.0f, 1.0f) * 65535.0f;
            vertex.atlasRect[corner] = (uint16_t) (corner < 2 ? ceilf(scaled) : floorf(scaled));
            vertex.rawLight[corner]  = PackVertexLightChannel(rawLights[i], corner);
        }

        TextureAtlas::Region region = {rect[0], rect[1], rect[2], rect[3]};
        if (atlas && atlas->IsAnimated(region))
        {
            AppendUniqueRegion(&data.atlasRegions, region);
        }
    }

    for (uint32_t i = 0; i < data.vertexCount; i += 6)
    {
        const float *p0 = vertices + (size_t) i * 8;
        const float *p1 = vertices + (size_t) std::min(i + 1, data.vertexCount - 1) * 8;
        const float *p2 = vertices + (size_t) std::min(i + 2, data.vertexCount - 1) * 8;
        glm::dvec3 origin(p0[0], p0[1], p0[2]);
        glm::dvec3 normal = glm::normalize(glm::cross((glm::dvec3(p1[0], p1[1], p1[2]) - origin), glm::dvec3(p2[0], p2[1], p2[2]) - origin));
        double octX       = normal.x / (fabs(normal.x) + fabs(normal.y) + fabs(normal.z));
        double octY       = normal.y / (fabs(normal.x) + fabs(normal.y) + fabs(normal.z));
        if (normal.z < 0.0)
        {
            double foldedX = (1.0 - fabs(octY)) * (octX >= 0.0 ? 1.0 : -1.0);
            double foldedY = (1.0 - fabs(octX)) * (octY >= 0.0 ? 1.0 : -1.0);
            octX           = foldedX;
            octY           = foldedY;
        }

        for (uint32_t j = 0; j < 6 && i + j < data.vertexCount; j++)
        {
            data.vertices[i + j].position[3] = (uint16_t) (PackBiasedNormalByte(octX) | (PackBiasedNormalByte(octY) << 8));
        }
    }

    uint32_t quadCount = data.vertexCount / 6;
    for (uint32_t q = 0; q < quadCount; q++)
    {
        data.vertices[q * 4 + 0] = data.vertices[q * 6 + 0];
        data.vertices[q * 4 + 1] = data.vertices[q * 6 + 1];
        data.vertices[q * 4 + 2] = data.vertices[q * 6 + 2];
        data.vertices[q * 4 + 3] = data.vertices[q * 6 + 4];
    }

    data.vertexCount = quadCount * 4;
    data.vertices.resize(data.vertexCount);
    data.vertices.shrink_to_fit();
    return data;
}

void ChunkMesh::AppendUniqueRegion(std::vector<TextureAtlas::Region> *regions, const TextureAtlas::Region &region)
{
    for (size_t i = 0; i < regions->size(); i++)
    {
        const TextureAtlas::Region &existing = (*regions)[i];
        if (existing.u0 == region.u0 && existing.v0 == region.v0 && existing.u1 == region.u1 && existing.v1 == region.v1)
        {
            return;
        }
    }

    regions->push_back(region);
}

void ChunkMesh::UpdateVertices(int firstVertex, const std::vector<Vertex> &vertices)
{
    ChunkVertexArena::GetInstance()->Upload(m_page, m_first + firstVertex, vertices.data(), (uint32_t) vertices.size());
}

void ChunkMesh::BindQuadIndices(uint32_t quadCount)
{
    RenderDevice *device = RenderDevice::GetInstance();
    if (!s_indexBuffer.IsValid())
    {
        s_indexBuffer = device->CreateBuffer();
    }

    device->BindElementArrayBuffer(s_indexBuffer);
    if (quadCount <= s_indexQuadCapacity)
    {
        return;
    }

    s_indexQuadCapacity = std::max(quadCount, std::max(s_indexQuadCapacity * 2, (uint32_t) 16384));
    std::vector<uint32_t> indices((size_t) s_indexQuadCapacity * 6);
    for (uint32_t q = 0; q < s_indexQuadCapacity; q++)
    {
        indices[q * 6 + 0] = q * 4 + 0;
        indices[q * 6 + 1] = q * 4 + 1;
        indices[q * 6 + 2] = q * 4 + 2;
        indices[q * 6 + 3] = q * 4 + 2;
        indices[q * 6 + 4] = q * 4 + 3;
        indices[q * 6 + 5] = q * 4 + 0;
    }

    device->UploadElementArrayBuffer(indices.data(), (uint32_t) (indices.size() * sizeof(uint32_t)), eBufferUsage_Static);
}

void ChunkMesh::Allocate(uint32_t capacity)
{
    ChunkVertexArena *arena = ChunkVertexArena::GetInstance();
    m_vertexCount           = 0;
    if (m_capacity > 0)
    {
        arena->Free(m_page, m_first, m_capacity);
    }

    SetCapacity(capacity);
    if (capacity > 0)
    {
        m_first = arena->Allocate(capacity, &m_page);
    }
}

void ChunkMesh::CopyVertices(const ChunkMesh &source, int sourceFirst, int destinationFirst, int count)
{
    ChunkVertexArena *arena = ChunkVertexArena::GetInstance();
    RenderDevice::GetInstance()->CopyArrayBuffer(
            arena->GetBuffer(source.m_page), arena->GetBuffer(m_page), (source.m_first + sourceFirst) * (uint32_t) sizeof(Vertex),
            (m_first + destinationFirst) * (uint32_t) sizeof(Vertex), (uint32_t) count * (uint32_t) sizeof(Vertex));
}

void ChunkMesh::SetVertexCount(uint32_t vertexCount)
{
    m_vertexCount = vertexCount;
}

const std::vector<TextureAtlas::Region> &ChunkMesh::GetAtlasRegions() const
{
    return m_atlasRegions;
}

void ChunkMesh::SetCapacity(uint32_t capacity)
{
    s_gpuBytes -= (uint64_t) m_capacity * sizeof(Vertex);
    s_gpuBytes += (uint64_t) capacity * sizeof(Vertex);
    m_capacity = capacity;
}

uint64_t ChunkMesh::GetGpuBytes()
{
    return s_gpuBytes.load();
}

uint32_t ChunkMesh::GetCapacity() const
{
    return m_capacity;
}

void ChunkMesh::AddAtlasRegions(const std::vector<TextureAtlas::Region> &regions)
{
    for (size_t i = 0; i < regions.size(); i++)
    {
        AppendUniqueRegion(&m_atlasRegions, regions[i]);
    }
}

uint32_t ChunkMesh::GetVertexCount() const
{
    return m_vertexCount;
}

uint32_t ChunkMesh::GetPage() const
{
    return m_page;
}

uint32_t ChunkMesh::GetFirstVertex() const
{
    return m_first;
}

bool ChunkMesh::IsTranslucent() const
{
    return m_translucent;
}

bool ChunkMesh::SortsQuads() const
{
    return m_sortQuads;
}

Texture *ChunkMesh::GetTexture() const
{
    return m_texture;
}

void ChunkMesh::AppendAtlasRegions(std::vector<TextureAtlas::Region> *regions) const
{
    for (size_t i = 0; i < m_atlasRegions.size(); i++)
    {
        AppendUniqueRegion(regions, m_atlasRegions[i]);
    }
}

size_t ChunkMesh::GetCpuBytes() const
{
    return m_atlasRegions.capacity() * sizeof(TextureAtlas::Region);
}

#endif
