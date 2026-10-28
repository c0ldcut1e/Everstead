#pragma once

#include <cstdint>

#include <glm/mat4x4.hpp>

class Lighting
{
public:
    static void TurnOn();
    static void TurnOff();
    static bool IsEnabled();
    static void SetEntityLight(uint32_t lightColor);
    static uint32_t GetEntityLight();
    static float GetDiffuse(const glm::dmat4 &matrix, float normalX, float normalY, float normalZ);
    static uint32_t ShadeColor(uint32_t color, float brightness);
    static void SetEyeLights(const glm::dmat4 &view);
    static void ClearEyeLights();
    static bool HasEyeLights();
    static float GetEyeDiffuse(float normalX, float normalY, float normalZ);

private:
    static uint32_t s_entityLight;
    static bool s_enabled;
    static bool s_eyeLights;
    static float s_eyeLight0[3];
    static float s_eyeLight1[3];
};
