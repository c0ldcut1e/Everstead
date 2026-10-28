#pragma once

#include "../WorldProvider.h"

#include <glm/vec3.hpp>

class ItemLights;
class Level;

class LightSource
{
public:
    static float SampleSkyLightClamp(const WorldProvider &worldProvider);
    static float SampleSkyDarken(const WorldProvider &worldProvider, float rainLevel, float thunderLevel);
    static float SampleBrightnessRamp(uint8_t lightLevel);
    static float SampleBlockLightScale(float blockFlicker);
    static void TickDarkAdaptation(Level *level, const glm::dvec3 &eyePos, const ItemLights &itemLights);
    static glm::dvec3 SampleLightColor(uint8_t blockR, uint8_t blockG, uint8_t blockB, uint8_t sky, float skyDarken);

    static float s_blockFlicker;
    static float s_darkAdaptation;

private:
    static const float s_blockLightBaseScale;
    static const float s_blockLightFlickerScale;
};
