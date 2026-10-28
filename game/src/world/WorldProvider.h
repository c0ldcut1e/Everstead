#pragma once

#include <cstdint>

#include "environment/Fog.h"

#include <glm/vec3.hpp>

class WorldProvider
{
public:
    static const int s_dayTicks              = 24000;
    static const int s_defaultDarkStartTick  = 12000;
    static const int s_defaultDarkPeakTick   = 13000;
    static const int64_t s_defaultStartTicks = 1500;

    WorldProvider();

    void TickWorldTime();
    void SetWorldTime(int64_t ticks);
    int64_t GetWorldTime() const;

    float GetDayFraction() const;
    float GetCelestialAngle() const;
    float GetDaylightFactor() const;
    glm::dvec3 GetSunDirection() const;
    glm::dvec3 GetMoonDirection() const;
    uint8_t GetSkyLightClamp() const;

    void SetDaylightCurve(int darkStartTick, int darkPeakTick);
    int GetDarkStartTick() const;
    int GetDarkPeakTick() const;

    Fog &GetFog();
    const Fog &GetFog() const;

private:
    void Recompute();

    int64_t m_worldTime;
    int m_darkStartTick;
    int m_darkPeakTick;

    float m_dayFraction;
    float m_celestialAngle;
    float m_daylightFactor;
    glm::dvec3 m_sunDirection;
    glm::dvec3 m_moonDirection;
    uint8_t m_skyLightClamp;
    Fog m_fog;
};
