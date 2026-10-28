#include "Icon.h"

Icon::Icon()
{
    m_texture = nullptr;
    m_u0      = 0.0f;
    m_v0      = 0.0f;
    m_u1      = 1.0f;
    m_v1      = 1.0f;
}

Icon::Icon(Texture *texture)
{
    m_texture = texture;
    m_u0      = 0.0f;
    m_v0      = 0.0f;
    m_u1      = 1.0f;
    m_v1      = 1.0f;
}

Icon::Icon(Texture *texture, float u0, float v0, float u1, float v1)
{
    m_texture = texture;
    m_u0      = u0;
    m_v0      = v0;
    m_u1      = u1;
    m_v1      = v1;
}

void Icon::SetTexture(Texture *texture)
{
    m_texture = texture;
}

Texture *Icon::GetTexture() const
{
    return m_texture;
}

void Icon::SetUV(float u0, float v0, float u1, float v1)
{
    m_u0 = u0;
    m_v0 = v0;
    m_u1 = u1;
    m_v1 = v1;
}

float Icon::GetU0() const
{
    return m_u0;
}

float Icon::GetV0() const
{
    return m_v0;
}

float Icon::GetU1() const
{
    return m_u1;
}

float Icon::GetV1() const
{
    return m_v1;
}

bool Icon::IsEmpty() const
{
    return m_texture == nullptr;
}
