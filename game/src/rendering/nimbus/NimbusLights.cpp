#ifndef EVERSTEAD_DEDICATED_SERVER

#include "NimbusLights.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Textures.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

NimbusLights::NimbusLights(int width, int height, const NimbusSettings *settings, const DynamicLights *dynamicLights)
{
    m_settings      = settings;
    m_dynamicLights = dynamicLights;
    m_tileTexture   = {};
    Resize(width, height);
}

NimbusLights::~NimbusLights()
{
    RenderDevice::GetInstance()->DeleteTexture(m_tileTexture);
}

void NimbusLights::Resize(int width, int height)
{
    if (m_tileTexture.IsValid())
    {
        RenderDevice::GetInstance()->DeleteTexture(m_tileTexture);
    }

    m_width  = width;
    m_height = height;
    m_tilesX = (width + kTileSize - 1) / kTileSize;
    m_tilesY = (height + kTileSize - 1) / kTileSize;
    m_tileMasks.assign((size_t) m_tilesX * m_tilesY, 0);
    m_tileTexture = Textures::Create(m_tilesX, m_tilesY, eTextureFormat_R32UI, eTextureWrap_ClampToEdge, m_tileMasks.data());
}

// Each light gets a bit in every screen tile its bounds touch, so pixels only loop over lights that can reach them
void NimbusLights::Cull(const glm::dvec3 &cameraPos, const glm::dmat4 &viewProjection)
{
    std::fill(m_tileMasks.begin(), m_tileMasks.end(), 0);
    for (size_t i = 0; i < m_dynamicLights->GetLights().size(); i++)
    {
        const DynamicLights::Light &light = m_dynamicLights->GetLights()[i];
        int minX                          = 0;
        int minY                          = 0;
        int maxX                          = m_tilesX - 1;
        int maxY                          = m_tilesY - 1;
        if (light.type != DynamicLights::eLightType_Directional)
        {
            glm::dvec3 center = light.position - cameraPos;
            double radius     = light.radius;
            glm::dvec2 ndcMin(std::numeric_limits<double>::max());
            glm::dvec2 ndcMax(-std::numeric_limits<double>::max());
            bool crossesCamera = false;
            for (int corner = 0; corner < 8 && !crossesCamera; corner++)
            {
                glm::dvec3 offset((corner & 1) ? radius : -radius, (corner & 2) ? radius : -radius, (corner & 4) ? radius : -radius);
                glm::dvec4 clip = viewProjection * glm::dvec4(center + offset, 1.0);
                crossesCamera   = clip.w <= 0.001;
                ndcMin          = glm::min(ndcMin, glm::dvec2(clip) / clip.w);
                ndcMax          = glm::max(ndcMax, glm::dvec2(clip) / clip.w);
            }

            if (!crossesCamera)
            {
                if (ndcMax.x < -1.0 || ndcMax.y < -1.0 || ndcMin.x > 1.0 || ndcMin.y > 1.0)
                {
                    continue;
                }

                minX = std::clamp((int) floor((ndcMin.x * 0.5 + 0.5) * m_width / kTileSize), 0, m_tilesX - 1);
                minY = std::clamp((int) floor((ndcMin.y * 0.5 + 0.5) * m_height / kTileSize), 0, m_tilesY - 1);
                maxX = std::clamp((int) floor((ndcMax.x * 0.5 + 0.5) * m_width / kTileSize), 0, m_tilesX - 1);
                maxY = std::clamp((int) floor((ndcMax.y * 0.5 + 0.5) * m_height / kTileSize), 0, m_tilesY - 1);
            }
        }

        for (int y = minY; y <= maxY; y++)
        {
            for (int x = minX; x <= maxX; x++)
            {
                m_tileMasks[(size_t) y * m_tilesX + x] |= (uint32_t) 1 << i;
            }
        }
    }

    RenderDevice::GetInstance()->UploadTextureRegion(m_tileTexture, 0, 0, m_tilesX, m_tilesY, eTextureFormat_R32UI, m_tileMasks.data());
}

void NimbusLights::Bind(Shader *shader, const glm::dvec3 &cameraPos) const
{
    float positions[DynamicLights::kMaxLights * 4];
    float colors[DynamicLights::kMaxLights * 4];
    float directions[DynamicLights::kMaxLights * 4];
    float params[DynamicLights::kMaxLights * 4];
    int count      = (int) m_dynamicLights->GetLights().size();
    int sunChannel = 0;
    for (int i = 0; i < count; i++)
    {
        const DynamicLights::Light &light = m_dynamicLights->GetLights()[(size_t) i];
        glm::vec3 direction               = glm::normalize(light.direction);
        bool occluded                     = m_settings->occlusion && light.occlusion;
        positions[i * 4]                  = (float) (light.position.x - cameraPos.x);
        positions[i * 4 + 1]              = (float) (light.position.y - cameraPos.y);
        positions[i * 4 + 2]              = (float) (light.position.z - cameraPos.z);
        positions[i * 4 + 3]              = light.radius;
        colors[i * 4]                     = light.color.x * light.intensity;
        colors[i * 4 + 1]                 = light.color.y * light.intensity;
        colors[i * 4 + 2]                 = light.color.z * light.intensity;
        colors[i * 4 + 3]                 = (float) light.type;
        directions[i * 4]                 = direction.x;
        directions[i * 4 + 1]             = direction.y;
        directions[i * 4 + 2]             = direction.z;
        directions[i * 4 + 3]             = cosf(light.coneAngle * 0.5f * std::numbers::pi_v<float> / 180.0f);
        params[i * 4]                     = light.size;
        params[i * 4 + 1]                 = light.volumetric && m_settings->volumetricLight ? 1.0f : 0.0f;
        params[i * 4 + 2]                 = -1.0f;
        params[i * 4 + 3]                 = occluded ? 1.0f : 0.0f;
        if (light.type == DynamicLights::eLightType_Directional && occluded && sunChannel < DynamicLights::kMaxSuns)
        {
            params[i * 4 + 2] = (float) sunChannel;
            sunChannel++;
        }
    }

    shader->SetInt("u_nimbusLightCount", count);
    shader->SetVec4Array("u_nimbusLightPositions", count, positions);
    shader->SetVec4Array("u_nimbusLightColors", count, colors);
    shader->SetVec4Array("u_nimbusLightDirections", count, directions);
    shader->SetVec4Array("u_nimbusLightParams", count, params);
    shader->SetFloat("u_nimbusFloodDecay", FloodDecay());
}

// The flood has one decay for every light, so we pick the one that lets the widest light reach about 2 percent at its radius
float NimbusLights::FloodDecay() const
{
    float radius = m_settings->blockLighting ? 15.0f : 1.0f;
    for (size_t i = 0; i < m_dynamicLights->GetLights().size(); i++)
    {
        if (m_dynamicLights->GetLights()[i].type != DynamicLights::eLightType_Directional)
        {
            radius = std::max(radius, m_dynamicLights->GetLights()[i].radius);
        }
    }

    return std::clamp((2.0f * logf(50.0f) - logf(radius)) / std::max(radius - 1.0f, 1.0f), 0.05f, 2.0f);
}

bool NimbusLights::HasVolumetricLight() const
{
    if (!m_settings->volumetricLight)
    {
        return false;
    }

    if (m_settings->blockLighting)
    {
        return true;
    }

    for (size_t i = 0; i < m_dynamicLights->GetLights().size(); i++)
    {
        if (m_dynamicLights->GetLights()[i].volumetric)
        {
            return true;
        }
    }

    return false;
}

TextureHandle NimbusLights::GetTileTexture() const
{
    return m_tileTexture;
}

#endif
