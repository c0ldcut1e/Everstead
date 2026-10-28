#include "WorldProvider.h"

#include <cmath>
#include <numbers>

WorldProvider::WorldProvider()
{
    m_worldTime      = s_defaultStartTicks;
    m_darkStartTick  = s_defaultDarkStartTick;
    m_darkPeakTick   = s_defaultDarkPeakTick;
    m_dayFraction    = 0.0f;
    m_celestialAngle = 0.0f;
    m_daylightFactor = 1.0f;
    m_sunDirection   = {1.0f, 0.0f, 0.0f};
    m_moonDirection  = {-1.0f, 0.0f, 0.0f};
    m_skyLightClamp  = 15;
    Recompute();
}

void WorldProvider::TickWorldTime()
{
    m_worldTime++;
    Recompute();
}

void WorldProvider::SetWorldTime(int64_t ticks)
{
    m_worldTime = ticks;
    Recompute();
}

int64_t WorldProvider::GetWorldTime() const
{
    return m_worldTime;
}

float WorldProvider::GetDayFraction() const
{
    return m_dayFraction;
}

float WorldProvider::GetCelestialAngle() const
{
    return m_celestialAngle;
}

float WorldProvider::GetDaylightFactor() const
{
    return m_daylightFactor;
}

glm::dvec3 WorldProvider::GetSunDirection() const
{
    return m_sunDirection;
}

glm::dvec3 WorldProvider::GetMoonDirection() const
{
    return m_moonDirection;
}

uint8_t WorldProvider::GetSkyLightClamp() const
{
    return m_skyLightClamp;
}

int WorldProvider::GetDarkStartTick() const
{
    return m_darkStartTick;
}

int WorldProvider::GetDarkPeakTick() const
{
    return m_darkPeakTick;
}

Fog &WorldProvider::GetFog()
{
    return m_fog;
}

const Fog &WorldProvider::GetFog() const
{
    return m_fog;
}

void WorldProvider::SetDaylightCurve(int darkStartTick, int darkPeakTick)
{
    darkStartTick = ((darkStartTick % s_dayTicks) + s_dayTicks) % s_dayTicks;
    darkPeakTick  = ((darkPeakTick % s_dayTicks) + s_dayTicks) % s_dayTicks;

    m_darkStartTick = darkStartTick;
    m_darkPeakTick  = darkPeakTick;

    Recompute();
}

void WorldProvider::Recompute()
{
    int tickInDay = (int) (m_worldTime % (int64_t) s_dayTicks);
    if (tickInDay < 0)
    {
        tickInDay += s_dayTicks;
    }

    m_dayFraction = (float) tickInDay / (float) s_dayTicks;

    {
        float shifted = m_dayFraction - 0.25f;
        if (shifted < 0.0f)
        {
            shifted += 1.0f;
        }

        float eased      = 1.0f - (float) ((cos((double) shifted * std::numbers::pi) + 1.0) * 0.5);
        m_celestialAngle = shifted + (eased - shifted) / 3.0f;
    }

    {
        int duskLength = m_darkPeakTick - m_darkStartTick;
        if (duskLength < 0)
        {
            duskLength += s_dayTicks;
        }

        if (duskLength < 1)
        {
            m_daylightFactor = 1.0f;
        }
        else
        {
            const int dayPeakTick   = (m_darkPeakTick + 12000) % s_dayTicks;
            const int dawnStartTick = ((dayPeakTick - duskLength) % s_dayTicks + s_dayTicks) % s_dayTicks;
            const int nightLength   = ((dawnStartTick - m_darkPeakTick) % s_dayTicks + s_dayTicks) % s_dayTicks;

            const int fromDusk  = ((tickInDay - m_darkStartTick) % s_dayTicks + s_dayTicks) % s_dayTicks;
            const int fromNight = ((tickInDay - m_darkPeakTick) % s_dayTicks + s_dayTicks) % s_dayTicks;
            const int fromDawn  = ((tickInDay - dawnStartTick) % s_dayTicks + s_dayTicks) % s_dayTicks;

            if (fromDusk < duskLength)
            {
                float progress   = (float) fromDusk / (float) duskLength;
                m_daylightFactor = 0.5f + 0.5f * cosf(progress * std::numbers::pi_v<float>);
            }
            else if (fromNight < nightLength)
            {
                m_daylightFactor = 0.0f;
            }
            else if (fromDawn < duskLength)
            {
                float progress   = (float) fromDawn / (float) duskLength;
                m_daylightFactor = 0.5f - 0.5f * cosf(progress * std::numbers::pi_v<float>);
            }
            else
            {
                m_daylightFactor = 1.0f;
            }
        }
    }

    {
        const float celestialRadians = m_celestialAngle * 6.28318530718f;
        m_sunDirection               = glm::dvec3((double) cosf(celestialRadians), (double) sinf(celestialRadians), 0.0);
        m_moonDirection              = glm::dvec3(-(double) cosf(celestialRadians), -(double) sinf(celestialRadians), 0.0);
    }

    {
        int duskLength = m_darkPeakTick - m_darkStartTick;
        if (duskLength < 0)
        {
            duskLength += s_dayTicks;
        }

        if (duskLength < 1)
        {
            m_skyLightClamp = 15;
        }
        else
        {
            const int dayPeakTick   = (m_darkPeakTick + 12000) % s_dayTicks;
            const int dawnStartTick = ((dayPeakTick - duskLength) % s_dayTicks + s_dayTicks) % s_dayTicks;
            const int nightLength   = ((dawnStartTick - m_darkPeakTick) % s_dayTicks + s_dayTicks) % s_dayTicks;

            const int fromDusk  = ((tickInDay - m_darkStartTick) % s_dayTicks + s_dayTicks) % s_dayTicks;
            const int fromNight = ((tickInDay - m_darkPeakTick) % s_dayTicks + s_dayTicks) % s_dayTicks;
            const int fromDawn  = ((tickInDay - dawnStartTick) % s_dayTicks + s_dayTicks) % s_dayTicks;

            int clamp;
            if (fromDusk < duskLength)
            {
                clamp = 15 - (fromDusk * 15) / duskLength;
                if (clamp < 0)
                {
                    clamp = 0;
                }
            }
            else if (fromNight < nightLength)
            {
                clamp = 0;
            }
            else if (fromDawn < duskLength)
            {
                clamp = (fromDawn * 15) / duskLength;
                if (clamp > 15)
                {
                    clamp = 15;
                }
            }
            else
            {
                clamp = 15;
            }

            m_skyLightClamp = (uint8_t) clamp;
        }
    }
}
