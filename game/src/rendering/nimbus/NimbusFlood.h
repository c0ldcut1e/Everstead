#pragma once

#include <cstdint>
#include <vector>

#include "../../world/DynamicLights.h"
#include "../../world/lighting/ItemLights.h"
#include "../RenderTypes.h"
#include "NimbusGrid.h"
#include "NimbusLights.h"
#include "NimbusSettings.h"

#include <glm/vec3.hpp>

class Framebuffer;
class Shader;

class NimbusFlood
{
public:
    NimbusFlood(const NimbusSettings *settings, NimbusGrid *grid, const NimbusLights *lights, const DynamicLights *dynamicLights,
                const ItemLights *itemLights);
    ~NimbusFlood();

    bool Render(const glm::dvec3 &cameraPos);
    void Clear();
    void MarkEmissionDirty();
    TextureHandle GetTexture() const;
    const glm::ivec3 &GetMin() const;
    int GetLightCount() const;
    double GetFade(const glm::dvec3 &pos) const;
    glm::dvec3 SampleSnapshot(const glm::dvec3 &pos) const;

private:
    static constexpr int kSize       = 128;
    static constexpr int kMaxBounces = 64;

    void UpdateEmission(const glm::ivec3 &floodMin);
    void WriteEmitters(int slot);
    void UpdateSnapshot();

    const NimbusSettings *m_settings;
    NimbusGrid *m_grid;
    const NimbusLights *m_lights;
    const DynamicLights *m_dynamicLights;
    const ItemLights *m_itemLights;
    Framebuffer *m_framebuffers[2];
    int m_current;
    Shader *m_shader;
    glm::ivec3 m_min;
    int m_lightCount;
    std::vector<uint32_t> m_emission;
    TextureHandle m_emissionTexture;
    glm::ivec3 m_emissionMin;
    bool m_emissionDirty;
    const uint8_t *m_snapshot;
    glm::ivec3 m_snapshotMin;
    glm::ivec3 m_readbackMin;
    bool m_readbackPending;
    int m_readbackFrame;
};
