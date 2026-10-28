#pragma once

#include <cstdint>
#include <vector>

#include "RenderTypes.h"

class Tesselator;

class BufferBuilder
{
public:
    struct Vtx
    {
        float x;
        float y;
        float z;
        float u;
        float v;
        float r;
        float g;
        float b;
        float a;
    };

    BufferBuilder();
    explicit BufferBuilder(Tesselator *owner);

    void SetOwner(Tesselator *owner);

    void Begin(ePRIMITIVE_TYPE mode);
    void End();

    void Color(uint32_t argb);
    void Color(float r, float g, float b, float a);

    void Vertex(float x, float y, float z);
    void VertexUV(float x, float y, float z, float u, float v);

    void Clear();

    ePRIMITIVE_TYPE GetMode() const;
    const std::vector<Vtx> &GetVertices() const;

private:
    Tesselator *m_owner;
    std::vector<Vtx> m_vertices;
    ePRIMITIVE_TYPE m_mode;
    float m_currentColor[4];
};
