#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LightSource.h"

#include <algorithm>
#include <cmath>

#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../WorldProvider.h"
#include "ItemLights.h"
#include "LightEngine.h"

float LightSource::s_blockFlicker   = 0.0f;
float LightSource::s_darkAdaptation = 0.0f;

const float LightSource::s_blockLightBaseScale    = 1.5f;
const float LightSource::s_blockLightFlickerScale = 0.45f;

float LightSource::SampleSkyLightClamp(const WorldProvider &worldProvider)
{
    int tickInDay = (int) (worldProvider.GetWorldTime() % (int64_t) WorldProvider::s_dayTicks);
    if (tickInDay < 0)
    {
        tickInDay += WorldProvider::s_dayTicks;
    }

    int darkStartTick = worldProvider.GetDarkStartTick();
    int darkPeakTick  = worldProvider.GetDarkPeakTick();
    int duskLength    = darkPeakTick - darkStartTick;
    if (duskLength < 0)
    {
        duskLength += WorldProvider::s_dayTicks;
    }

    if (duskLength < 1)
    {
        return 15.0f;
    }

    // Dawn mirrors dusk around noon so the ground brightens together with the sky
    int dawnStartTick = ((12000 - darkPeakTick) % WorldProvider::s_dayTicks + WorldProvider::s_dayTicks) % WorldProvider::s_dayTicks;
    int nightLength   = ((dawnStartTick - darkPeakTick) % WorldProvider::s_dayTicks + WorldProvider::s_dayTicks) % WorldProvider::s_dayTicks;
    int fromDusk      = ((tickInDay - darkStartTick) % WorldProvider::s_dayTicks + WorldProvider::s_dayTicks) % WorldProvider::s_dayTicks;
    int fromNight     = ((tickInDay - darkPeakTick) % WorldProvider::s_dayTicks + WorldProvider::s_dayTicks) % WorldProvider::s_dayTicks;
    int fromDawn      = ((tickInDay - dawnStartTick) % WorldProvider::s_dayTicks + WorldProvider::s_dayTicks) % WorldProvider::s_dayTicks;
    if (fromDusk < duskLength)
    {
        return 15.0f - (float) fromDusk * 15.0f / (float) duskLength;
    }

    if (fromNight < nightLength)
    {
        return 0.0f;
    }

    if (fromDawn < duskLength)
    {
        return (float) fromDawn * 15.0f / (float) duskLength;
    }

    return 15.0f;
}

float LightSource::SampleSkyDarken(const WorldProvider &worldProvider, float rainLevel, float thunderLevel)
{
    float td = worldProvider.GetCelestialAngle();
    float br = 1.0f - (cosf(td * 6.28318530718f) * 2.0f + 0.2f);
    br       = std::clamp(br, 0.0f, 1.0f);
    br       = 1.0f - br;
    br *= 1.0f - rainLevel * 5.0f / 16.0f;
    br *= 1.0f - thunderLevel * 5.0f / 16.0f;
    return br * 0.8f + 0.2f;
}

float LightSource::SampleBrightnessRamp(uint8_t lightLevel)
{
    float level = std::clamp((float) lightLevel, 0.0f, 15.0f) / 15.0f;
    float v     = 1.0f - level;
    return (1.0f - v) / (v * 3.0f + 1.0f);
}

float LightSource::SampleBlockLightScale(float blockFlicker)
{
    return s_blockLightBaseScale + blockFlicker * s_blockLightFlickerScale;
}

void LightSource::TickDarkAdaptation(Level *level, const glm::dvec3 &eyePos, const ItemLights &itemLights)
{
    BlockPos pos((int) floor(eyePos.x), (int) floor(eyePos.y), (int) floor(eyePos.z));
    uint8_t blockR;
    uint8_t blockG;
    uint8_t blockB;
    LightEngine::GetBlockLight(level, pos, &blockR, &blockG, &blockB);
    itemLights.SampleBlockLight(eyePos, &blockR, &blockG, &blockB);

    float skyDarken = SampleSkyDarken(level->GetWorldProvider(), level->GetRainLevel(1.0f), level->GetThunderLevel(1.0f));
    float sky       = (float) level->GetSkyLightLevel(pos) * (skyDarken - 0.2f) / 0.8f;
    float block     = (float) std::max(blockR, std::max(blockG, blockB));
    float target    = std::clamp((block - sky - 4.0f) / 8.0f, 0.0f, 1.0f);
    s_darkAdaptation += (target - s_darkAdaptation) * 0.05f;
}

glm::dvec3 LightSource::SampleLightColor(uint8_t blockR, uint8_t blockG, uint8_t blockB, uint8_t sky, float skyDarken)
{
    float blockScale   = SampleBlockLightScale(s_blockFlicker);
    float darken       = skyDarken * 0.95f + 0.05f;
    float skyValue     = SampleBrightnessRamp(sky) * darken * (1.0f - s_darkAdaptation * 0.4f);
    float ambientFloor = 0.03f * (1.0f - s_darkAdaptation);
    float blockRed     = SampleBrightnessRamp(blockR) * blockScale;
    float blockGreen   = SampleBrightnessRamp(blockG) * blockScale;
    float blockBlue    = SampleBrightnessRamp(blockB) * blockScale;
    float rs           = skyValue * (skyDarken * 0.65f + 0.35f);
    float gs           = skyValue * (skyDarken * 0.65f + 0.35f);
    float bs           = skyValue;
    float rb           = blockRed;
    float gb           = blockGreen * ((blockGreen * 0.6f + 0.4f) * 0.6f + 0.4f);
    float bb           = blockBlue * ((blockBlue * blockBlue) * 0.6f + 0.4f);
    float r            = rs + rb;
    float g            = gs + gb;
    float b            = bs + bb;
    r                  = r * 0.96f + ambientFloor;
    g                  = g * 0.96f + ambientFloor;
    b                  = b * 0.96f + ambientFloor;
    if (r > 1.0f)
    {
        r = 1.0f;
    }

    if (g > 1.0f)
    {
        g = 1.0f;
    }

    if (b > 1.0f)
    {
        b = 1.0f;
    }

    r = r * 0.96f + ambientFloor;
    g = g * 0.96f + ambientFloor;
    b = b * 0.96f + ambientFloor;
    if (r > 1.0f)
    {
        r = 1.0f;
    }

    if (g > 1.0f)
    {
        g = 1.0f;
    }

    if (b > 1.0f)
    {
        b = 1.0f;
    }

    if (r < 0.0f)
    {
        r = 0.0f;
    }

    if (g < 0.0f)
    {
        g = 0.0f;
    }

    if (b < 0.0f)
    {
        b = 0.0f;
    }

    return glm::dvec3(r, g, b);
}

#endif
