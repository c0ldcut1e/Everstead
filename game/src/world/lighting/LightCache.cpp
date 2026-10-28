#include "LightCache.h"

#include <cstdint>

LightCache::LightCache()
{
    m_skyLightClamp  = -1.0f;
    m_skyLightLevels = {};
    Rebuild(15.0f);
}

void LightCache::Rebuild(float skyLightClamp)
{
    if (skyLightClamp < 0.0f)
    {
        skyLightClamp = 0.0f;
    }

    if (skyLightClamp > 15.0f)
    {
        skyLightClamp = 15.0f;
    }

    if (m_skyLightClamp == skyLightClamp)
    {
        return;
    }

    m_skyLightClamp = skyLightClamp;

    for (int i = 0; i < 16; i++)
    {
        float level = (float) i;
        if (level > m_skyLightClamp)
        {
            level = m_skyLightClamp;
        }

        m_skyLightLevels[(uint32_t) i] = level / 15.0f;
    }
}

float LightCache::GetSkyLightClamp() const
{
    return m_skyLightClamp;
}

const std::array<float, 16> &LightCache::GetSkyLightLevels() const
{
    return m_skyLightLevels;
}
