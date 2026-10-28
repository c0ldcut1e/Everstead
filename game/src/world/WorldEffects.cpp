#include "WorldEffects.h"

#include <algorithm>
#include <cmath>

#include "../utils/Time.h"

#include <glm/gtc/matrix_transform.hpp>

static const WorldEffects::Definition kDefinitions[WorldEffects::eWorldEffectType_Count] = {
        {2400, glm::vec3(0.7f, 0.3f, 1.0f), 8.0f, 4.0f, 1200, 2.5f, 8.0f, glm::vec3(0.9f, 0.75f, 1.0f), 0.85f, 6.0f},
        {6000, glm::vec3(1.0f, 0.45f, 0.6f), 40.0f, 5.0f, 4000, 14.0f, 12.0f, glm::vec3(1.0f, 0.88f, 0.85f), 0.8f, 8.0f},
        {800, glm::vec3(0.0f), 0.0f, 0.0f, 0, 0.0f, 1.0f, glm::vec3(0.0f), 0.0f, 1.0f},
        {8200, glm::vec3(0.8f, 0.6f, 1.0f), 16.0f, 3.0f, 8200, 5.0f, 4.0f, glm::vec3(1.0f, 0.4f, 0.95f), 0.6f, 4.0f},
        {1800, glm::vec3(0.4f, 0.9f, 1.0f), 14.0f, 3.0f, 900, 4.5f, 5.0f, glm::vec3(0.5f, 0.9f, 1.0f), 0.45f, 4.0f},
        {8000, glm::vec3(1.0f, 0.7f, 1.0f), 48.0f, 6.0f, 5000, 16.0f, 10.0f, glm::vec3(1.0f, 0.85f, 1.0f), 0.9f, 8.0f},
        {6200, glm::vec3(1.0f, 0.6f, 0.3f), 10.0f, 3.0f, 6000, 2.0f, 4.0f, glm::vec3(0.2f, 0.1f, 0.3f), 0.5f, 4.0f},
        {6000, glm::vec3(0.7f, 0.8f, 1.0f), 48.0f, 6.0f, 4000, 18.0f, 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 0.95f, 8.0f},
        {6200, glm::vec3(1.0f, 0.5f, 0.2f), 24.0f, 4.0f, 6200, 2.0f, 4.0f, glm::vec3(1.0f, 0.6f, 0.3f), 0.4f, 4.0f},
        {7000, glm::vec3(1.0f, 0.8f, 0.5f), 64.0f, 7.0f, 5000, 20.0f, 10.0f, glm::vec3(1.0f, 0.9f, 0.7f), 1.0f, 9.0f},
        {6200, glm::vec3(0.5f, 0.6f, 1.0f), 18.0f, 4.0f, 6200, 3.0f, 4.0f, glm::vec3(0.7f, 0.8f, 1.0f), 0.5f, 4.0f},
        {5000, glm::vec3(0.7f, 0.8f, 1.0f), 56.0f, 6.0f, 3000, 16.0f, 10.0f, glm::vec3(0.9f, 0.95f, 1.0f), 0.95f, 8.0f},
        {6200, glm::vec3(1.0f, 0.8f, 1.0f), 16.0f, 3.0f, 6200, 1.5f, 4.0f, glm::vec3(1.0f, 0.8f, 1.0f), 0.4f, 4.0f},
        {6000, glm::vec3(1.0f, 1.0f, 1.0f), 48.0f, 6.0f, 4000, 14.0f, 10.0f, glm::vec3(1.0f, 1.0f, 1.0f), 0.9f, 8.0f},
        {15200, glm::vec3(1.0f, 0.08f, 0.04f), 14.0f, 4.0f, 15200, 3.0f, 6.0f, glm::vec3(0.25f, 0.0f, 0.0f), 0.7f, 6.0f},
        {10000, glm::vec3(1.0f, 0.15f, 0.05f), 72.0f, 8.0f, 7000, 24.0f, 12.0f, glm::vec3(1.0f, 0.1f, 0.05f), 1.0f, 10.0f},
        {450, glm::vec3(0.0f), 0.0f, 0.0f, 0, 1.5f, 1.0f, glm::vec3(0.5f, 0.3f, 1.0f), 0.25f, 1.0f},
        {900, glm::vec3(0.6f, 0.35f, 1.0f), 6.0f, 2.0f, 700, 1.0f, 1.5f, glm::vec3(0.0f), 0.0f, 1.0f},
        {700, glm::vec3(0.8f, 0.9f, 1.0f), 4.0f, 3.0f, 300, 0.8f, 4.0f, glm::vec3(0.0f), 0.0f, 1.0f},
        {2600, glm::vec3(0.7f, 0.4f, 1.0f), 5.0f, 6.0f, 2600, 0.0f, 1.0f, glm::vec3(0.0f), 0.0f, 1.0f},
};

void WorldEffects::Add(eWORLD_EFFECT_TYPE type, const glm::dvec3 &center, float radius)
{
    if (m_effects.size() >= kMaxEffects)
    {
        return;
    }

    m_effects.push_back({type, center, radius, (float) m_random.NextInt(0, 1000), Time::GetMilliseconds(), 0, false});
}

void WorldEffects::Update(DynamicLights *dynamicLights)
{
    uint32_t now = Time::GetMilliseconds();
    size_t i     = 0;
    while (i < m_effects.size())
    {
        Effect &effect               = m_effects[i];
        const Definition &definition = kDefinitions[effect.type];
        uint32_t age                 = now - effect.startTime;
        if (!effect.lit && definition.lightIntensity > 0.0f)
        {
            effect.lit = true;
            do
            {
                effect.lightId = m_random.NextUInt();
            } while (effect.lightId == 0 || dynamicLights->GetLight(effect.lightId));

            DynamicLights::Light light = {effect.lightId,
                                          DynamicLights::eLightType_Point,
                                          effect.center,
                                          glm::vec3(0.0f, -1.0f, 0.0f),
                                          definition.lightColor,
                                          definition.lightIntensity,
                                          effect.radius * definition.lightRange,
                                          0.0f,
                                          1.0f,
                                          true,
                                          true};
            if (!dynamicLights->SetLight(light))
            {
                effect.lightId = 0;
            }
        }

        const DynamicLights::Light *current = effect.lightId != 0 ? dynamicLights->GetLight(effect.lightId) : nullptr;
        if (current && (age >= definition.lightDuration || age >= definition.duration))
        {
            dynamicLights->RemoveLight(effect.lightId);
            effect.lightId = 0;
        }
        else if (current)
        {
            float fade                 = 1.0f - (float) age / (float) definition.lightDuration;
            DynamicLights::Light light = *current;
            light.intensity            = definition.lightIntensity * fade * fade * (0.55f + 0.45f * m_random.NextFloat());
            dynamicLights->SetLight(light);
        }
        else
        {
            effect.lightId = 0;
        }

        if (age >= definition.duration)
        {
            m_effects.erase(m_effects.begin() + (ptrdiff_t) i);
        }
        else
        {
            i++;
        }
    }
}

const std::vector<WorldEffects::Effect> &WorldEffects::GetEffects() const
{
    return m_effects;
}

float WorldEffects::GetProgress(const Effect &effect, uint32_t now) const
{
    return std::min((float) (now - effect.startTime) / (float) kDefinitions[effect.type].duration, 1.0f);
}

glm::dmat4 WorldEffects::GetShake(const glm::dvec3 &cameraPos) const
{
    uint32_t now = Time::GetMilliseconds();
    glm::vec3 shake(0.0f);
    for (size_t i = 0; i < m_effects.size(); i++)
    {
        const Effect &effect         = m_effects[i];
        const Definition &definition = kDefinitions[effect.type];
        float age                    = (float) (now - effect.startTime) / 1000.0f;
        float falloff  = std::max(1.0f - (float) glm::distance(cameraPos, effect.center) / (effect.radius * definition.shakeRange), 0.0f);
        float strength = definition.shake * expf(-age * 3.5f) * falloff * falloff;
        shake += glm::vec3(sinf(age * 67.0f + effect.seed), sinf(age * 53.0f + effect.seed * 2.1f), sinf(age * 41.0f + effect.seed * 3.3f) * 0.6f) *
                 strength;
    }

    glm::dmat4 transform = glm::rotate(glm::dmat4(1.0), (double) glm::radians(shake.x), glm::dvec3(1.0, 0.0, 0.0));
    transform            = glm::rotate(transform, (double) glm::radians(shake.y), glm::dvec3(0.0, 1.0, 0.0));
    return glm::rotate(transform, (double) glm::radians(shake.z), glm::dvec3(0.0, 0.0, 1.0));
}

glm::vec4 WorldEffects::GetFlash(const glm::dvec3 &cameraPos) const
{
    uint32_t now = Time::GetMilliseconds();
    glm::vec4 flash(0.0f);
    for (size_t i = 0; i < m_effects.size(); i++)
    {
        const Effect &effect         = m_effects[i];
        const Definition &definition = kDefinitions[effect.type];
        float age                    = (float) (now - effect.startTime) / 1000.0f;
        float falloff = std::max(1.0f - (float) glm::distance(cameraPos, effect.center) / (effect.radius * definition.flashRange), 0.0f);
        float amount  = definition.flash * expf(-age * 5.0f) * falloff;
        if (amount > flash.a)
        {
            flash = glm::vec4(definition.flashColor, amount);
        }
    }

    return flash;
}
