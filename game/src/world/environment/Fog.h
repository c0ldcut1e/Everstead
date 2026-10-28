#pragma once

#include <glm/vec3.hpp>

class Fog
{
public:
    Fog();

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    float GetStartFactor() const;
    float GetEndFactor() const;
    void SetRangeFactors(float startFactor, float endFactor);

    const glm::dvec3 &GetColor() const;
    void SetColor(const glm::dvec3 &color);
    void SetColor(float r, float g, float b);

    const glm::dvec3 &GetSkyColor() const;
    void SetSkyColor(const glm::dvec3 &skyColor);

private:
    bool m_enabled;

    float m_startFactor;
    float m_endFactor;

    glm::dvec3 m_color;

    glm::dvec3 m_skyColor;
};
