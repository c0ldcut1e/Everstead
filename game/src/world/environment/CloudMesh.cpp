#ifndef EVERSTEAD_DEDICATED_SERVER

#include "CloudMesh.h"

#include "../../rendering/GlStateManager.h"
#include "../../rendering/RenderDevice.h"
#include "../../rendering/Texture.h"
#include "../../rendering/Textures.h"

#include <glm/gtc/type_ptr.hpp>

CloudMesh::CloudMesh()
{
    RenderDevice *device = RenderDevice::GetInstance();
    m_vertexCount        = 0;
    m_shader.Load("assets/shader/cloud.vert", "assets/shader/cloud.frag");
    m_vao = device->CreateVertexArray();
    m_vbo = device->CreateBuffer();

    device->BindVertexArray(m_vao);
    device->BindArrayBuffer(m_vbo);
    device->SetVertexAttrib(0, 3, eVertexAttribType_Float, false, 9 * sizeof(float), 0);
    device->SetVertexAttrib(1, 2, eVertexAttribType_Float, false, 9 * sizeof(float), 3 * sizeof(float));
    device->SetVertexAttrib(2, 4, eVertexAttribType_Float, false, 9 * sizeof(float), 5 * sizeof(float));
}

CloudMesh::~CloudMesh()
{
    RenderDevice::GetInstance()->DeleteBuffer(m_vbo);
    RenderDevice::GetInstance()->DeleteVertexArray(m_vao);
}

void CloudMesh::Upload(const std::vector<float> &vertices)
{
    m_vertexCount = (uint32_t) (vertices.size() / 9);

    RenderDevice *device = RenderDevice::GetInstance();
    device->BindVertexArray(m_vao);
    device->BindArrayBuffer(m_vbo);
    device->UploadArrayBuffer(vertices.data(), (uint32_t) (vertices.size() * sizeof(float)), eBufferUsage_Static);
}

void CloudMesh::Prepare(Texture *texture, const glm::dmat4 &model, const glm::dmat4 &view, const glm::dmat4 &projection, float red, float green,
                        float blue, float alphaCutoff, float uvOffsetX, float uvOffsetZ, const glm::vec4 &flash) const
{
    m_shader.Bind();
    m_shader.SetMat4("u_model", glm::value_ptr(model));
    m_shader.SetMat4("u_view", glm::value_ptr(view));
    m_shader.SetMat4("u_projection", glm::value_ptr(projection));
    m_shader.SetVec3("u_cloudColor", red, green, blue);
    m_shader.SetVec2("u_uvOffset", uvOffsetX, uvOffsetZ);
    m_shader.SetVec4("u_flash", flash.x, flash.y, flash.z, flash.w);
    m_shader.SetFloat("u_alphaCutoff", alphaCutoff);

    bool fogEnabled = GlStateManager::IsFogEnabled();
    m_shader.SetInt("u_fogEnabled", fogEnabled ? 1 : 0);
    if (fogEnabled)
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

    Textures::Bind(texture->GetId());
}

void CloudMesh::Render(int firstVertex, int vertexCount) const
{
    if (!m_vertexCount || !vertexCount)
    {
        return;
    }

    RenderDevice::GetInstance()->BindVertexArray(m_vao);
    RenderDevice::GetInstance()->DrawArrays(ePrimitiveType_Triangles, firstVertex, vertexCount);
}

#endif
