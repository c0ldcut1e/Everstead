#pragma once

class Texture;

class Icon
{
public:
    Icon();
    explicit Icon(Texture *texture);
    Icon(Texture *texture, float u0, float v0, float u1, float v1);

    void SetTexture(Texture *texture);
    Texture *GetTexture() const;

    void SetUV(float u0, float v0, float u1, float v1);
    float GetU0() const;
    float GetV0() const;
    float GetU1() const;
    float GetV1() const;

    bool IsEmpty() const;

private:
    Texture *m_texture;
    float m_u0;
    float m_v0;
    float m_u1;
    float m_v1;
};
