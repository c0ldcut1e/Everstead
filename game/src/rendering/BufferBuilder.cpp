#ifndef EVERSTEAD_DEDICATED_SERVER

#include "BufferBuilder.h"

#include "Tesselator.h"

BufferBuilder::BufferBuilder()
{
    m_owner           = nullptr;
    m_mode            = ePrimitiveType_Triangles;
    m_currentColor[0] = 1.0f;
    m_currentColor[1] = 1.0f;
    m_currentColor[2] = 1.0f;
    m_currentColor[3] = 1.0f;
}

BufferBuilder::BufferBuilder(Tesselator *owner)
{
    m_owner           = owner;
    m_mode            = ePrimitiveType_Triangles;
    m_currentColor[0] = 1.0f;
    m_currentColor[1] = 1.0f;
    m_currentColor[2] = 1.0f;
    m_currentColor[3] = 1.0f;
}

void BufferBuilder::SetOwner(Tesselator *owner)
{
    m_owner = owner;
}

void BufferBuilder::Begin(ePRIMITIVE_TYPE mode)
{
    m_mode = mode;
    m_vertices.clear();
}

void BufferBuilder::End()
{
    if (!m_owner)
    {
        return;
    }

    m_owner->Flush(*this);
}

void BufferBuilder::Color(uint32_t argb)
{
    float a           = ((argb >> 24) & 0xFF) / 255.0f;
    float r           = ((argb >> 16) & 0xFF) / 255.0f;
    float g           = ((argb >> 8) & 0xFF) / 255.0f;
    float b           = (argb & 0xFF) / 255.0f;
    m_currentColor[0] = r;
    m_currentColor[1] = g;
    m_currentColor[2] = b;
    m_currentColor[3] = a;
}

void BufferBuilder::Color(float r, float g, float b, float a)
{
    m_currentColor[0] = r;
    m_currentColor[1] = g;
    m_currentColor[2] = b;
    m_currentColor[3] = a;
}

void BufferBuilder::Vertex(float x, float y, float z)
{
    VertexUV(x, y, z, 0.0f, 0.0f);
}

void BufferBuilder::VertexUV(float x, float y, float z, float u, float v)
{
    struct Vtx vtx;
    vtx.x = x;
    vtx.y = y;
    vtx.z = z;
    vtx.u = u;
    vtx.v = v;
    vtx.r = m_currentColor[0];
    vtx.g = m_currentColor[1];
    vtx.b = m_currentColor[2];
    vtx.a = m_currentColor[3];
    m_vertices.push_back(vtx);
}

void BufferBuilder::Clear()
{
    m_vertices.clear();
}

ePRIMITIVE_TYPE BufferBuilder::GetMode() const
{
    return m_mode;
}

const std::vector<BufferBuilder::Vtx> &BufferBuilder::GetVertices() const
{
    return m_vertices;
}

#endif
