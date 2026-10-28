#include "Fog.h"

Fog::Fog()
{
    m_enabled     = true;
    m_startFactor = 0.25f;
    m_endFactor   = 1.0f;
    m_color       = {191.0 / 255.0, 212.0 / 255.0, 255.0 / 255.0};
    m_skyColor    = {127.0 / 255.0, 167.0 / 255.0, 255.0 / 255.0};
}

bool Fog::IsEnabled() const
{
    return m_enabled;
}

void Fog::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

float Fog::GetStartFactor() const
{
    return m_startFactor;
}

float Fog::GetEndFactor() const
{
    return m_endFactor;
}

void Fog::SetRangeFactors(float startFactor, float endFactor)
{
    m_startFactor = startFactor;
    m_endFactor   = endFactor;
}

const glm::dvec3 &Fog::GetColor() const
{
    return m_color;
}

void Fog::SetColor(const glm::dvec3 &color)
{
    m_color = color;
}

void Fog::SetColor(float r, float g, float b)
{
    m_color = glm::dvec3(r, g, b);
}

const glm::dvec3 &Fog::GetSkyColor() const
{
    return m_skyColor;
}

void Fog::SetSkyColor(const glm::dvec3 &skyColor)
{
    m_skyColor = skyColor;
}
