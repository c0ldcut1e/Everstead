#pragma once

#include "../../world/DynamicLights.h"
#include "NimbusFlood.h"
#include "NimbusGrid.h"
#include "NimbusLights.h"
#include "NimbusSettings.h"
#include "NimbusSunCache.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Framebuffer;
class Shader;

class NimbusVolume
{
public:
    NimbusVolume(int width, int height, const NimbusSettings *settings, const NimbusGrid *grid, const NimbusLights *lights, const NimbusFlood *flood,
                 const NimbusSunCache *sunCache, const DynamicLights *dynamicLights);
    ~NimbusVolume();

    void Resize(int width, int height);
    void Render(Framebuffer *sceneFramebuffer, const glm::dmat4 &invViewProjection, const glm::dvec3 &cameraPos, double time);
    Shader *GetVolumeShader() const;
    Shader *GetApplyShader() const;

private:
    const NimbusSettings *m_settings;
    const NimbusGrid *m_grid;
    const NimbusLights *m_lights;
    const NimbusFlood *m_flood;
    const NimbusSunCache *m_sunCache;
    const DynamicLights *m_dynamicLights;
    Framebuffer *m_spotShadowFramebuffer;
    Shader *m_spotShadowShader;
    Framebuffer *m_froxelScatterFramebuffer;
    Framebuffer *m_froxelFramebuffer;
    glm::ivec3 m_froxelGrid;
    Shader *m_volumeShader;
    Shader *m_integrateShader;
    Framebuffer *m_volumeFramebuffer;
    Shader *m_applyShader;
    Shader *m_compositeShader;
};
