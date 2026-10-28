#pragma once

#include "../../world/DynamicLights.h"
#include "../../world/chunk/SectionPos.h"
#include "../RenderTypes.h"
#include "NimbusGrid.h"
#include "NimbusSettings.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Framebuffer;
class Shader;

class NimbusSunCache
{
public:
    NimbusSunCache(const NimbusSettings *settings, const NimbusGrid *grid, const DynamicLights *dynamicLights);
    ~NimbusSunCache();

    bool Update(const glm::dvec3 &cameraPos);
    void Invalidate();
    TextureHandle GetTexture() const;
    int GetSlotsBuilt() const;

private:
    void MarkShadowed(const SectionPos &pos);

    const NimbusSettings *m_settings;
    const NimbusGrid *m_grid;
    const DynamicLights *m_dynamicLights;
    glm::vec4 m_suns[DynamicLights::kMaxSuns];
    int m_sunCount;
    bool m_dirty[NimbusGrid::kSlotCount];
    int m_slotsBuilt;
    bool m_cleared;
    Framebuffer *m_framebuffer;
    Shader *m_shader;
};
