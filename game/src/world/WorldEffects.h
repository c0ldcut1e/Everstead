#pragma once

#include <cstdint>
#include <vector>

#include "../utils/Random.h"
#include "DynamicLights.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class WorldEffects
{
public:
    static constexpr size_t kMaxEffects = 16;

    // These are read by world_effect.frag, so new types have to be added there too
    enum eWORLD_EFFECT_TYPE : int
    {
        eWorldEffectType_RiftShockwave       = 0,
        eWorldEffectType_RiftDetonation      = 1,
        eWorldEffectType_RiftImplosion       = 2,
        eWorldEffectType_ExperimentTesseract = 3,
        eWorldEffectType_ExperimentBurst     = 4,
        eWorldEffectType_ExperimentFinale    = 5,
        eWorldEffectType_SingularityCharge   = 6,
        eWorldEffectType_SingularityBlast    = 7,
        eWorldEffectType_NovaCharge          = 8,
        eWorldEffectType_NovaBlast           = 9,
        eWorldEffectType_TempestCharge       = 10,
        eWorldEffectType_TempestBlast        = 11,
        eWorldEffectType_PrismCharge         = 12,
        eWorldEffectType_PrismBlast          = 13,
        eWorldEffectType_HellgateCharge      = 14,
        eWorldEffectType_HellgateBlast       = 15,
        eWorldEffectType_PhaseGlitch         = 16,
        eWorldEffectType_FracturePulse       = 17,
        eWorldEffectType_FractureCrack       = 18,
        eWorldEffectType_FractureTear        = 19,
        eWorldEffectType_Count
    };

    struct Definition
    {
        uint32_t duration;
        glm::vec3 lightColor;
        float lightIntensity;
        float lightRange;
        uint32_t lightDuration;
        float shake;
        float shakeRange;
        glm::vec3 flashColor;
        float flash;
        float flashRange;
    };

    struct Effect
    {
        eWORLD_EFFECT_TYPE type;
        glm::dvec3 center;
        float radius;
        float seed;
        uint32_t startTime;
        uint32_t lightId;
        bool lit;
    };

    void Add(eWORLD_EFFECT_TYPE type, const glm::dvec3 &center, float radius);
    void Update(DynamicLights *dynamicLights);
    const std::vector<Effect> &GetEffects() const;
    float GetProgress(const Effect &effect, uint32_t now) const;
    glm::dmat4 GetShake(const glm::dvec3 &cameraPos) const;
    glm::vec4 GetFlash(const glm::dvec3 &cameraPos) const;

private:
    std::vector<Effect> m_effects;
    Random m_random;
};
