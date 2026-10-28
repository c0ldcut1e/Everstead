#include "DynamicLights.h"

DynamicLights::DynamicLights()
{
    m_airDensity = 0.004f;
    m_anisotropy = 0.6f;
}

const std::vector<DynamicLights::Light> &DynamicLights::GetLights() const
{
    return m_lights;
}

const DynamicLights::Light *DynamicLights::GetLight(uint32_t id) const
{
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        if (m_lights[i].id == id)
        {
            return &m_lights[i];
        }
    }

    return nullptr;
}

bool DynamicLights::SetLight(const Light &light)
{
    int suns     = 0;
    size_t index = m_lights.size();
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        if (m_lights[i].id == light.id)
        {
            index = i;
        }
        else if (m_lights[i].type == eLightType_Directional)
        {
            suns++;
        }
    }

    if (light.id == 0 || (light.type == eLightType_Directional && suns >= kMaxSuns) || (index == m_lights.size() && index >= kMaxLights))
    {
        return false;
    }

    if (index == m_lights.size())
    {
        m_lights.push_back(light);
    }
    else
    {
        m_lights[index] = light;
    }

    return true;
}

void DynamicLights::RemoveLight(uint32_t id)
{
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        if (m_lights[i].id == id)
        {
            m_lights.erase(m_lights.begin() + i);
            return;
        }
    }
}

const std::vector<DynamicLights::FogVolume> &DynamicLights::GetFogVolumes() const
{
    return m_fogVolumes;
}

const DynamicLights::FogVolume *DynamicLights::GetFogVolume(uint32_t id) const
{
    for (size_t i = 0; i < m_fogVolumes.size(); i++)
    {
        if (m_fogVolumes[i].id == id)
        {
            return &m_fogVolumes[i];
        }
    }

    return nullptr;
}

bool DynamicLights::SetFogVolume(const FogVolume &fog)
{
    if (fog.id == 0)
    {
        return false;
    }

    for (size_t i = 0; i < m_fogVolumes.size(); i++)
    {
        if (m_fogVolumes[i].id == fog.id)
        {
            m_fogVolumes[i] = fog;
            return true;
        }
    }

    if (m_fogVolumes.size() >= kMaxFogVolumes)
    {
        return false;
    }

    m_fogVolumes.push_back(fog);
    return true;
}

void DynamicLights::RemoveFogVolume(uint32_t id)
{
    for (size_t i = 0; i < m_fogVolumes.size(); i++)
    {
        if (m_fogVolumes[i].id == id)
        {
            m_fogVolumes.erase(m_fogVolumes.begin() + i);
            return;
        }
    }
}

float DynamicLights::GetAirDensity() const
{
    return m_airDensity;
}

float DynamicLights::GetAnisotropy() const
{
    return m_anisotropy;
}

void DynamicLights::SetAtmosphere(float airDensity, float anisotropy)
{
    m_airDensity = airDensity;
    m_anisotropy = anisotropy;
}

void DynamicLights::Clear()
{
    *this = DynamicLights();
}
