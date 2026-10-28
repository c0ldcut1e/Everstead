#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "BufferBuilder.h"
#include "Shader.h"

#include <glm/mat4x4.hpp>

class Tesselator
{
public:
    static Tesselator *GetInstance();

    BufferBuilder *GetBuilder();

    void BeginBatch();
    void EndBatch();
    // Ghost draws only go through batches, so this has to be set between BeginBatch and EndBatch
    void SetGhostAlpha(float alpha);

private:
    friend class BufferBuilder;

    Tesselator();

    struct BatchState
    {
        TextureHandle texture;
        ePRIMITIVE_TYPE mode;
        float alphaCutoff;
        float polygonOffsetFactor;
        float polygonOffsetUnits;
        uint32_t overlayColor;
        float glint;
        eBLEND_FACTOR blendSrc;
        eBLEND_FACTOR blendDst;
        bool cull;
        bool blend;
        bool colorWrite;
        bool polygonOffset;
        bool textureEnabled;
        bool depthTest;
        bool depthMask;
    };

    struct Batch
    {
        BatchState state;
        std::vector<BufferBuilder::Vtx> vertices;
    };

    void Flush(const BufferBuilder &builder);
    void Draw(const std::vector<BufferBuilder::Vtx> &vertices, ePRIMITIVE_TYPE mode, const glm::dmat4 &model);

    VertexArrayHandle m_vao;
    BufferHandle m_vbo;

    Shader m_shader;
    BufferBuilder m_builder;

    std::vector<Batch> m_batches;
    size_t m_batchCount;
    bool m_batching;
    float m_ghostAlpha;
};
