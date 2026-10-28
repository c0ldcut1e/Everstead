#pragma once

#include <cstdint>

#include "../../utils/Random.h"
#include "../../world/DynamicLights.h"
#include "../../world/chunk/SectionPos.h"
#include "../../world/lighting/ItemLights.h"
#include "NimbusClouds.h"
#include "NimbusFlood.h"
#include "NimbusGrid.h"
#include "NimbusLights.h"
#include "NimbusSettings.h"
#include "NimbusSunCache.h"
#include "NimbusVolume.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

constexpr bool kNimbusEnabled = true;

class Framebuffer;
class Level;
class Shader;
class Texture;

class Nimbus
{
public:
    Nimbus(int width, int height, DynamicLights *dynamicLights, const ItemLights *itemLights);
    ~Nimbus();

    void Resize(int width, int height);

    void Update(Level *level, const glm::dvec3 &cameraPos, const glm::dmat4 &viewProjection);
    void MarkSectionDirty(const SectionPos &pos);
    void BindSurface(Shader *shader, const glm::dvec3 &cameraPos) const;
    void RenderVolume(Framebuffer *sceneFramebuffer, const glm::dmat4 &invViewProjection, const glm::dvec3 &cameraPos);
    void RenderClouds(Framebuffer *outputFramebuffer, Texture *cloudTexture, const glm::dmat4 &invViewProjection, const glm::vec3 &cloudColor,
                      const glm::vec2 &cloudCell, float cloudBottom, const glm::vec4 &flash, float rain, const glm::vec3 &sunDirection,
                      float maxDistance);
    Shader *GetVolumeShader() const;
    Shader *GetApplyShader() const;
    Shader *GetCloudShader() const;
    glm::dvec3 SampleLight(const glm::dvec3 &pos, const glm::dvec3 &baseLight) const;
    void RenderImGui(const glm::dvec3 &cameraPos);

    bool IsEnabled() const;
    bool IsBlockLighting() const;
    bool HasClouds() const;
    bool HasFog() const;
    float GetAtmosphere() const;
    double GetFloodFade(const glm::dvec3 &pos) const;

private:
    glm::dvec3 Shoulder(const glm::dvec3 &light) const;

    NimbusSettings m_settings;
    NimbusGrid *m_grid;
    NimbusLights *m_lights;
    NimbusSunCache *m_sunCache;
    NimbusFlood *m_flood;
    NimbusVolume *m_volume;
    NimbusClouds *m_clouds;

    DynamicLights *m_dynamicLights;
    uint32_t m_selectedLightId;
    uint32_t m_selectedFogId;
    Random m_random;
    int m_width;
    int m_height;
    double m_time;
};
