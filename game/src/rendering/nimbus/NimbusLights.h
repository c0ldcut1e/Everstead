#pragma once

#include <cstdint>
#include <vector>

#include "../../world/DynamicLights.h"
#include "../RenderTypes.h"
#include "NimbusSettings.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Shader;

class NimbusLights
{
public:
    static constexpr int kTileSize = 32;

    NimbusLights(int width, int height, const NimbusSettings *settings, const DynamicLights *dynamicLights);
    ~NimbusLights();

    void Resize(int width, int height);
    void Cull(const glm::dvec3 &cameraPos, const glm::dmat4 &viewProjection);
    void Bind(Shader *shader, const glm::dvec3 &cameraPos) const;
    float FloodDecay() const;
    bool HasVolumetricLight() const;
    TextureHandle GetTileTexture() const;

private:
    const NimbusSettings *m_settings;
    const DynamicLights *m_dynamicLights;
    int m_width;
    int m_height;
    int m_tilesX;
    int m_tilesY;
    std::vector<uint32_t> m_tileMasks;
    TextureHandle m_tileTexture;
};
