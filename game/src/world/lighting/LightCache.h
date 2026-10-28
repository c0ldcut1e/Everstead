#pragma once

#include <array>

class LightCache
{
public:
    LightCache();

    void Rebuild(float skyLightClamp);

    float GetSkyLightClamp() const;
    const std::array<float, 16> &GetSkyLightLevels() const;

private:
    float m_skyLightClamp;
    std::array<float, 16> m_skyLightLevels;
};
