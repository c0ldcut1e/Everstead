#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Tesselator.h"

#include "../utils/Time.h"
#include "GlStateManager.h"
#include "RenderDevice.h"
#include "Textures.h"

#include <glm/gtc/type_ptr.hpp>

Tesselator::Tesselator()
{
    m_shader.Load("assets/shader/buffer_builder.vert", "assets/shader/buffer_builder.frag");
    m_builder.SetOwner(this);
    m_batching   = false;
    m_batchCount = 0;
    m_ghostAlpha = 0.0f;

    RenderDevice *device = RenderDevice::GetInstance();
    m_vao                = device->CreateVertexArray();
    m_vbo                = device->CreateBuffer();
    device->BindVertexArray(m_vao);
    device->BindArrayBuffer(m_vbo);
    device->SetVertexAttrib(0, 3, eVertexAttribType_Float, false, sizeof(BufferBuilder::Vtx), 0);
    device->SetVertexAttrib(1, 2, eVertexAttribType_Float, false, sizeof(BufferBuilder::Vtx), 3 * sizeof(float));
    device->SetVertexAttrib(2, 4, eVertexAttribType_Float, false, sizeof(BufferBuilder::Vtx), 5 * sizeof(float));
}

Tesselator *Tesselator::GetInstance()
{
    // This static outlives the render device, so its buffers are freed with the context instead of in a destructor
    static Tesselator instance;
    return &instance;
}

BufferBuilder *Tesselator::GetBuilder()
{
    return &m_builder;
}

void Tesselator::BeginBatch()
{
    m_batching = true;
}

void Tesselator::SetGhostAlpha(float alpha)
{
    m_ghostAlpha = alpha;
}

// This draws each state group once with positions already in view space, in the order the groups first appeared
void Tesselator::EndBatch()
{
    m_batching = false;
    if (m_batchCount == 0)
    {
        return;
    }

    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    bool cull         = GlStateManager::IsCullEnabled();
    bool blend        = GlStateManager::IsBlendEnabled();
    bool colorWrite   = GlStateManager::IsColorWriteEnabled();
    bool offset       = GlStateManager::IsPolygonOffsetFillEnabled();
    bool texture      = GlStateManager::IsTextureEnabled();
    bool depthTest    = GlStateManager::IsDepthTestEnabled();
    bool depthMask    = GlStateManager::IsDepthMaskEnabled();
    uint32_t overlay  = GlStateManager::GetOverlayColor();
    float glint       = GlStateManager::GetGlint();
    eBLEND_FACTOR blendSrc;
    eBLEND_FACTOR blendDst;
    GlStateManager::GetBlendFunc(&blendSrc, &blendDst);
    float offsetFactor;
    float offsetUnits;
    GlStateManager::GetPolygonOffset(&offsetFactor, &offsetUnits);

    glm::dmat4 identity = glm::dmat4(1.0);
    for (size_t i = 0; i < m_batchCount; i++)
    {
        Batch &batch            = m_batches[i];
        const BatchState &state = batch.state;
        GlStateManager::SetAlphaCutoff(state.alphaCutoff);
        state.cull ? GlStateManager::EnableCull() : GlStateManager::DisableCull();
        state.blend ? GlStateManager::EnableBlend() : GlStateManager::DisableBlend();
        GlStateManager::SetColorMask(state.colorWrite, state.colorWrite, state.colorWrite, state.colorWrite);
        state.polygonOffset ? GlStateManager::EnablePolygonOffsetFill() : GlStateManager::DisablePolygonOffsetFill();
        GlStateManager::SetPolygonOffset(state.polygonOffsetFactor, state.polygonOffsetUnits);
        state.textureEnabled ? GlStateManager::EnableTexture() : GlStateManager::DisableTexture();
        state.depthTest ? GlStateManager::EnableDepthTest() : GlStateManager::DisableDepthTest();
        GlStateManager::SetDepthMask(state.depthMask);
        GlStateManager::SetBlendFunc(state.blendSrc, state.blendDst);
        GlStateManager::SetOverlayColor(state.overlayColor);
        GlStateManager::SetGlint(state.glint);
        Textures::Bind(state.texture);
        Draw(batch.vertices, state.mode, identity);
        batch.vertices.clear();
    }

    m_batchCount = 0;
    GlStateManager::SetAlphaCutoff(alphaCutoff);
    cull ? GlStateManager::EnableCull() : GlStateManager::DisableCull();
    blend ? GlStateManager::EnableBlend() : GlStateManager::DisableBlend();
    GlStateManager::SetColorMask(colorWrite, colorWrite, colorWrite, colorWrite);
    offset ? GlStateManager::EnablePolygonOffsetFill() : GlStateManager::DisablePolygonOffsetFill();
    GlStateManager::SetPolygonOffset(offsetFactor, offsetUnits);
    texture ? GlStateManager::EnableTexture() : GlStateManager::DisableTexture();
    depthTest ? GlStateManager::EnableDepthTest() : GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(depthMask);
    GlStateManager::SetBlendFunc(blendSrc, blendDst);
    GlStateManager::SetOverlayColor(overlay);
    GlStateManager::SetGlint(glint);
    Textures::Unbind();
}

void Tesselator::Flush(const BufferBuilder &builder)
{
    const std::vector<BufferBuilder::Vtx> &vertices = builder.GetVertices();
    if (vertices.empty())
    {
        return;
    }

    int oldMode = GlStateManager::GetMatrixMode();
    GlStateManager::MatrixMode(0);
    glm::dmat4 model = GlStateManager::GetMatrix();
    GlStateManager::MatrixMode(oldMode);

    // Strips and fans cannot be joined, so only lists are batched
    ePRIMITIVE_TYPE mode = builder.GetMode();
    bool ghost           = m_ghostAlpha > 0.0f;
    if (ghost && (!m_batching || (mode != ePrimitiveType_Triangles && mode != ePrimitiveType_Lines)))
    {
        return;
    }

    if (!m_batching || (mode != ePrimitiveType_Triangles && mode != ePrimitiveType_Lines))
    {
        Draw(vertices, mode, model);
        return;
    }

    BatchState state;
    state.texture        = Textures::GetBound();
    state.mode           = mode;
    state.alphaCutoff    = GlStateManager::GetAlphaCutoff();
    state.cull           = GlStateManager::IsCullEnabled();
    state.blend          = GlStateManager::IsBlendEnabled();
    state.colorWrite     = GlStateManager::IsColorWriteEnabled();
    state.polygonOffset  = GlStateManager::IsPolygonOffsetFillEnabled();
    state.textureEnabled = GlStateManager::IsTextureEnabled();
    state.depthTest      = GlStateManager::IsDepthTestEnabled();
    state.depthMask      = GlStateManager::IsDepthMaskEnabled();
    state.overlayColor   = GlStateManager::GetOverlayColor();
    state.glint          = GlStateManager::GetGlint();
    GlStateManager::GetBlendFunc(&state.blendSrc, &state.blendDst);
    GlStateManager::GetPolygonOffset(&state.polygonOffsetFactor, &state.polygonOffsetUnits);
    if (ghost)
    {
        state.blend        = true;
        state.blendSrc     = eBlendFactor_SrcAlpha;
        state.blendDst     = eBlendFactor_One;
        state.depthMask    = false;
        state.overlayColor = 0;
        state.glint        = 0.0f;
    }

    Batch *target = nullptr;
    for (size_t i = 0; i < m_batchCount; i++)
    {
        Batch &batch            = m_batches[i];
        const BatchState &other = batch.state;
        if (other.texture == state.texture && other.mode == state.mode && other.alphaCutoff == state.alphaCutoff && other.cull == state.cull &&
            other.blend == state.blend && other.colorWrite == state.colorWrite && other.polygonOffset == state.polygonOffset &&
            other.polygonOffsetFactor == state.polygonOffsetFactor && other.polygonOffsetUnits == state.polygonOffsetUnits &&
            other.textureEnabled == state.textureEnabled && other.depthTest == state.depthTest && other.depthMask == state.depthMask &&
            other.overlayColor == state.overlayColor && other.glint == state.glint && other.blendSrc == state.blendSrc &&
            other.blendDst == state.blendDst)
        {
            target = &batch;
            break;
        }
    }

    if (!target)
    {
        if (m_batchCount == m_batches.size())
        {
            m_batches.push_back({});
        }

        target        = &m_batches[m_batchCount];
        target->state = state;
        m_batchCount++;
    }

    const double *m = glm::value_ptr(model);
    for (const BufferBuilder::Vtx &vertex : vertices)
    {
        BufferBuilder::Vtx transformed = vertex;
        transformed.x                  = (float) (m[0] * vertex.x + m[4] * vertex.y + m[8] * vertex.z + m[12]);
        transformed.y                  = (float) (m[1] * vertex.x + m[5] * vertex.y + m[9] * vertex.z + m[13]);
        transformed.z                  = (float) (m[2] * vertex.x + m[6] * vertex.y + m[10] * vertex.z + m[14]);
        if (ghost)
        {
            transformed.r = vertex.r * 0.3f + 0.25f;
            transformed.g = vertex.g * 0.3f + 0.6f;
            transformed.b = vertex.b * 0.2f + 0.8f;
            transformed.a = vertex.a * m_ghostAlpha;
        }

        target->vertices.push_back(transformed);
    }
}

void Tesselator::Draw(const std::vector<BufferBuilder::Vtx> &vertices, ePRIMITIVE_TYPE mode, const glm::dmat4 &model)
{
    int oldMode = GlStateManager::GetMatrixMode();
    GlStateManager::MatrixMode(1);
    glm::dmat4 projection = GlStateManager::GetMatrix();
    GlStateManager::MatrixMode(oldMode);

    RenderDevice *device = RenderDevice::GetInstance();
    device->BindVertexArray(m_vao);
    device->BindArrayBuffer(m_vbo);
    device->UploadArrayBuffer(vertices.data(), (uint32_t) (vertices.size() * sizeof(BufferBuilder::Vtx)), eBufferUsage_Stream);

    m_shader.Bind();
    m_shader.SetMat4("u_view", glm::value_ptr(glm::dmat4(1.0)));
    m_shader.SetMat4("u_projection", glm::value_ptr(projection));
    m_shader.SetMat4("u_model", glm::value_ptr(model));
    m_shader.SetFloat("u_alphaCutoff", GlStateManager::GetAlphaCutoff());
    m_shader.SetInt("u_fogEnabled", GlStateManager::IsFogEnabled() ? 1 : 0);

    if (GlStateManager::IsFogEnabled())
    {
        float fogR;
        float fogG;
        float fogB;
        float fogStart;
        float fogEnd;
        GlStateManager::GetFogColor(&fogR, &fogG, &fogB);
        GlStateManager::GetFogRange(&fogStart, &fogEnd);
        m_shader.SetVec3("u_fogColor", fogR, fogG, fogB);
        m_shader.SetVec2("u_fogRange", fogStart, fogEnd);
    }

    m_shader.SetInt("u_useTexture", GlStateManager::IsTextureEnabled() ? 1 : 0);
    uint32_t overlay = GlStateManager::GetOverlayColor();
    m_shader.SetVec4("u_overlayColor", ((overlay >> 16) & 0xFF) / 255.0f, ((overlay >> 8) & 0xFF) / 255.0f, (overlay & 0xFF) / 255.0f,
                     ((overlay >> 24) & 0xFF) / 255.0f);
    m_shader.SetFloat("u_glint", GlStateManager::GetGlint());
    m_shader.SetFloat("u_time", (float) (Time::GetMilliseconds() % 600000) / 1000.0f);

    device->DrawArrays(mode, 0, (int) vertices.size());
}

#endif
