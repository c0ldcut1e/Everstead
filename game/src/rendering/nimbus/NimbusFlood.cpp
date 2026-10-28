#ifndef EVERSTEAD_DEDICATED_SERVER

#include "NimbusFlood.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../Framebuffer.h"
#include "../GlStateManager.h"
#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Textures.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/vector_relational.hpp>

NimbusFlood::NimbusFlood(const NimbusSettings *settings, NimbusGrid *grid, const NimbusLights *lights, const DynamicLights *dynamicLights,
                         const ItemLights *itemLights)
{
    m_settings      = settings;
    m_grid          = grid;
    m_lights        = lights;
    m_dynamicLights = dynamicLights;
    m_itemLights    = itemLights;
    for (int i = 0; i < 2; i++)
    {
        m_framebuffers[i] = new Framebuffer(kSize * 16, kSize * 8, eTextureFormat_RGBA16F);
        RenderDevice::GetInstance()->SetTextureFilter(m_framebuffers[i]->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    }

    m_current    = 0;
    m_shader     = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_flood.frag");
    m_min        = glm::ivec3(0);
    m_lightCount = 0;
    m_emission.assign((size_t) kSize * kSize * kSize, 0);
    m_emissionTexture = Textures::Create(kSize * 16, kSize * 8, eTextureFormat_RGBA8, eTextureWrap_ClampToEdge, m_emission.data());
    m_emissionMin     = glm::ivec3(0);
    m_emissionDirty   = true;
    m_snapshot        = nullptr;
    m_snapshotMin     = glm::ivec3(0);
    m_readbackMin     = glm::ivec3(0);
    m_readbackPending = false;
    m_readbackFrame   = 0;
}

NimbusFlood::~NimbusFlood()
{
    delete m_shader;
    delete m_framebuffers[1];
    delete m_framebuffers[0];
    RenderDevice::GetInstance()->DeleteTexture(m_emissionTexture);
}

bool NimbusFlood::Render(const glm::dvec3 &cameraPos)
{
    glm::ivec3 floodMin = glm::ivec3(glm::floor(cameraPos)) - kSize / 2;
    int count           = m_settings->blockLighting ? 1 : 0;
    for (size_t i = 0; i < m_dynamicLights->GetLights().size(); i++)
    {
        if (m_dynamicLights->GetLights()[i].type != DynamicLights::eLightType_Directional)
        {
            count++;
        }
    }

    bool clear   = m_lightCount == 0;
    m_lightCount = count;
    if (count == 0)
    {
        return false;
    }

    RenderDevice *device = RenderDevice::GetInstance();
    if (clear)
    {
        // Light left over from before the flood stopped would otherwise fade in at old positions
        GlStateManager::SetClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 2; i++)
        {
            m_framebuffers[i]->Bind();
            device->ClearAll();
        }
    }

    if (m_settings->blockLighting)
    {
        UpdateEmission(floodMin);
    }

    glm::ivec3 shift = floodMin - m_min;
    m_min            = floodMin;
    m_shader->Bind();
    Textures::Bind(m_grid->GetTexture(), 0);
    Textures::Bind(m_emissionTexture, 2);
    glm::ivec3 emissionOffset = floodMin - m_emissionMin;
    m_shader->SetVec3("u_floodEmissionOffset", (float) emissionOffset.x, (float) emissionOffset.y, (float) emissionOffset.z);
    m_shader->SetInt("u_floodEmission", m_settings->blockLighting ? 1 : 0);
    m_grid->Bind(m_shader, glm::dvec3(floodMin));
    m_lights->Bind(m_shader, glm::dvec3(floodMin));
    m_shader->SetInt("u_nimbusOcclusion", m_settings->occlusion || m_settings->blockLighting ? 1 : 0);
    m_shader->SetFloat("u_nimbusBounce", m_settings->bounce);

    // Spot lights only reach the flood through the spots they hit, so each one leaves a few bounce lights where its rays land
    constexpr int kBounceRays = 16;

    float bouncePositions[kMaxBounces * 4];
    float bounceColors[kMaxBounces * 4];
    int bounces  = 0;
    double decay = m_lights->FloodDecay();
    if (m_settings->blockLighting)
    {
        const std::vector<ItemLights::Light> &itemLights = m_itemLights->GetLights();
        for (size_t i = 0; i < itemLights.size(); i++)
        {
            glm::dvec3 position              = itemLights[i].position - glm::dvec3(floodMin);
            bouncePositions[bounces * 4]     = (float) position.x;
            bouncePositions[bounces * 4 + 1] = (float) position.y;
            bouncePositions[bounces * 4 + 2] = (float) position.z;
            bouncePositions[bounces * 4 + 3] = 0.0f;
            bounceColors[bounces * 4]        = (float) (itemLights[i].color.x / 15.0);
            bounceColors[bounces * 4 + 1]    = (float) (itemLights[i].color.y / 15.0);
            bounceColors[bounces * 4 + 2]    = (float) (itemLights[i].color.z / 15.0);
            bounceColors[bounces * 4 + 3]    = 0.0f;
            bounces++;
        }
    }

    for (size_t i = 0; i < m_dynamicLights->GetLights().size() && bounces + kBounceRays <= kMaxBounces; i++)
    {
        const DynamicLights::Light &light = m_dynamicLights->GetLights()[i];
        if (light.type != DynamicLights::eLightType_Spot || !light.occlusion || m_settings->bounce <= 0.0f)
        {
            continue;
        }

        glm::dvec3 forward = glm::normalize(glm::dvec3(light.direction));
        glm::dvec3 tangent = glm::normalize(glm::cross(forward, std::abs(forward.y) < 0.99 ? glm::dvec3(0.0, 1.0, 0.0) : glm::dvec3(1.0, 0.0, 0.0)));
        glm::dvec3 bitangent = glm::cross(forward, tangent);
        double coneCos       = cos(light.coneAngle * 0.5 * std::numbers::pi / 180.0);
        for (int ray = 0; ray < kBounceRays; ray++)
        {
            double cosTheta      = 1.0 - (1.0 - coneCos) * (ray + 0.5) / kBounceRays;
            double sinTheta      = sqrt(1.0 - cosTheta * cosTheta);
            double phi           = ray * 2.399963;
            glm::dvec3 direction = forward * cosTheta + (tangent * cos(phi) + bitangent * sin(phi)) * sinTheta;
            for (double distance = 0.25; distance < light.radius; distance += 0.25)
            {
                if (!m_grid->IsSolid(light.position + direction * distance))
                {
                    continue;
                }

                glm::dvec3 hit = light.position + direction * (distance - 0.5) - glm::dvec3(floodMin);
                double clamped = std::max(distance, 1.0);
                double window  = std::clamp(1.0 - pow(distance / light.radius, 4.0), 0.0, 1.0);
                double strength =
                        sqrt(exp(-decay * (clamped - 1.0)) / clamped) * window * window * light.intensity * sqrt(m_settings->bounce / kBounceRays);
                bouncePositions[bounces * 4]     = (float) hit.x;
                bouncePositions[bounces * 4 + 1] = (float) hit.y;
                bouncePositions[bounces * 4 + 2] = (float) hit.z;
                bouncePositions[bounces * 4 + 3] = 0.0f;
                bounceColors[bounces * 4]        = (float) (light.color.x * strength);
                bounceColors[bounces * 4 + 1]    = (float) (light.color.y * strength);
                bounceColors[bounces * 4 + 2]    = (float) (light.color.z * strength);
                bounceColors[bounces * 4 + 3]    = 0.0f;
                bounces++;
                break;
            }
        }
    }

    m_shader->SetInt("u_floodBounceCount", bounces);
    m_shader->SetVec4Array("u_floodBouncePositions", bounces, bouncePositions);
    m_shader->SetVec4Array("u_floodBounceColors", bounces, bounceColors);
    for (int i = 0; i < m_settings->floodIterations; i++)
    {
        // Only the first pass moves the volume with the camera since the later ones read cells it already shifted
        m_shader->SetVec3("u_floodShift", (float) shift.x, (float) shift.y, (float) shift.z);
        shift = glm::ivec3(0);
        m_framebuffers[1 - m_current]->Bind();
        device->SetViewport(0, 0, kSize * 16, kSize * 8);
        Textures::Bind(m_framebuffers[m_current]->GetColorTexture(), 1);
        device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
        m_current = 1 - m_current;
    }

    UpdateSnapshot();
    return true;
}

// Entities light themselves on the CPU, so we copy the flood back every few frames for them to sample the same light as the terrain
void NimbusFlood::UpdateSnapshot()
{
    const int readbackInterval = 4;

    RenderDevice *device    = RenderDevice::GetInstance();
    const uint8_t *snapshot = device->PollTextureReadback();
    if (snapshot)
    {
        m_snapshot        = snapshot;
        m_snapshotMin     = m_readbackMin;
        m_readbackPending = false;
    }

    m_readbackFrame++;
    if (!m_readbackPending && m_readbackFrame >= readbackInterval)
    {
        device->BeginTextureReadback(GetTexture(), kSize * 16, kSize * 8, eTextureFormat_RGBA16F);
        m_readbackMin     = m_min;
        m_readbackPending = true;
        m_readbackFrame   = 0;
    }
}

void NimbusFlood::UpdateEmission(const glm::ivec3 &floodMin)
{
    RenderDevice *device = RenderDevice::GetInstance();
    // We only recenter the emission volume once the flood has drifted a section away, since every recenter uploads all of it
    glm::ivec3 drift = glm::abs(floodMin - m_emissionMin);
    if (m_emissionDirty || drift.x > 16 || drift.y > 16 || drift.z > 16)
    {
        m_emissionMin   = floodMin;
        m_emissionDirty = false;
        m_grid->ClearEmitterSlots();
        std::fill(m_emission.begin(), m_emission.end(), 0);
        for (int slot = 0; slot < NimbusGrid::kSlotCount; slot++)
        {
            if (m_grid->GetSlot(slot).loaded)
            {
                WriteEmitters(slot);
            }
        }

        device->UploadTextureRegion(m_emissionTexture, 0, 0, kSize * 16, kSize * 8, eTextureFormat_RGBA8, m_emission.data());
        return;
    }

    std::vector<uint32_t> region;
    const std::vector<int> &emitterSlots = m_grid->GetEmitterSlots();
    for (size_t i = 0; i < emitterSlots.size(); i++)
    {
        int slot          = emitterSlots[i];
        SectionPos pos    = m_grid->GetSlot(slot).pos;
        glm::ivec3 origin = glm::ivec3(pos.x, pos.y, pos.z) * 16 - m_emissionMin;
        glm::ivec3 low    = glm::max(origin, glm::ivec3(0));
        glm::ivec3 high   = glm::min(origin + 16, glm::ivec3(kSize));
        if (glm::any(glm::greaterThanEqual(low, high)))
        {
            continue;
        }

        for (int z = low.z; z < high.z; z++)
        {
            for (int y = low.y; y < high.y; y++)
            {
                uint32_t *row = &m_emission[(size_t) ((z >> 4) * kSize + y) * kSize * 16 + (z & 15) * kSize];
                std::fill(row + low.x, row + high.x, 0);
            }
        }

        WriteEmitters(slot);
        int width  = high.x - low.x;
        int height = high.y - low.y;
        region.resize((size_t) width * height);
        for (int z = low.z; z < high.z; z++)
        {
            for (int y = low.y; y < high.y; y++)
            {
                const uint32_t *row = &m_emission[(size_t) ((z >> 4) * kSize + y) * kSize * 16 + (z & 15) * kSize];
                std::copy(row + low.x, row + high.x, &region[(size_t) (y - low.y) * width]);
            }

            device->UploadTextureRegion(m_emissionTexture, (z & 15) * kSize + low.x, (z >> 4) * kSize + low.y, width, height, eTextureFormat_RGBA8,
                                        region.data());
        }
    }

    m_grid->ClearEmitterSlots();
}

void NimbusFlood::WriteEmitters(int slot)
{
    const std::vector<NimbusGrid::Emitter> &emitters = m_grid->GetEmitters(slot);
    for (size_t i = 0; i < emitters.size(); i++)
    {
        glm::ivec3 cell = emitters[i].pos - m_emissionMin;
        if (glm::any(glm::lessThan(cell, glm::ivec3(0))) || glm::any(glm::greaterThanEqual(cell, glm::ivec3(kSize))))
        {
            continue;
        }

        glm::uvec3 rgb = glm::uvec3(emitters[i].color * 255.0f + 0.5f);
        m_emission[(size_t) ((cell.z >> 4) * kSize + cell.y) * kSize * 16 + (cell.z & 15) * kSize + cell.x] =
                rgb.r | (rgb.g << 8) | (rgb.b << 16) | 0xFF000000;
    }
}

void NimbusFlood::Clear()
{
    m_lightCount = 0;
}

void NimbusFlood::MarkEmissionDirty()
{
    m_emissionDirty = true;
}

TextureHandle NimbusFlood::GetTexture() const
{
    return m_framebuffers[m_current]->GetColorTexture();
}

const glm::ivec3 &NimbusFlood::GetMin() const
{
    return m_min;
}

int NimbusFlood::GetLightCount() const
{
    return m_lightCount;
}

double NimbusFlood::GetFade(const glm::dvec3 &pos) const
{
    glm::dvec3 offset = glm::abs(pos - glm::dvec3(m_min) - (double) (kSize / 2));
    return std::clamp((std::max(std::max(offset.x, offset.y), offset.z) - 48.0) / 16.0, 0.0, 1.0);
}

// This matches NimbusFloodSurfaceSample in nimbus_surface.glsl
glm::dvec3 NimbusFlood::SampleSnapshot(const glm::dvec3 &pos) const
{
    if (!m_snapshot)
    {
        return glm::dvec3(0.0);
    }

    const uint16_t *texels = (const uint16_t *) m_snapshot;
    glm::dvec3 cell        = glm::clamp(pos - glm::dvec3(m_snapshotMin), 0.5, kSize - 0.5) - 0.5;
    glm::ivec3 base        = glm::ivec3(cell);
    glm::dvec3 f           = cell - glm::dvec3(base);
    int home               = (int) (f.x >= 0.5) | ((int) (f.y >= 0.5) << 1) | ((int) (f.z >= 0.5) << 2);
    glm::dvec3 corners[8];
    uint32_t open = 0;
    for (int c = 0; c < 8; c++)
    {
        glm::ivec3 corner     = glm::min(base + glm::ivec3(c & 1, (c >> 1) & 1, c >> 2), glm::ivec3(kSize - 1));
        const uint16_t *texel = &texels[((size_t) ((corner.z >> 4) * kSize + corner.y) * kSize * 16 + (corner.z & 15) * kSize + corner.x) * 4];
        corners[c]            = glm::dvec3(glm::unpackHalf1x16(texel[0]), glm::unpackHalf1x16(texel[1]), glm::unpackHalf1x16(texel[2]));
        // A cell that just opened up reads as unlit until the next readback, so we only trust cells that hold some light
        if ((!m_settings->occlusion || !m_grid->IsSolid(glm::dvec3(m_snapshotMin + corner) + 0.5)) &&
            corners[c].x + corners[c].y + corners[c].z > 0.0)
        {
            open |= 1 << c;
        }
    }

    if (!((open >> home) & 1))
    {
        double best = 0.0;
        for (int c = 0; c < 8; c++)
        {
            glm::dvec3 w  = glm::mix(1.0 - f, f, glm::dvec3(c & 1, (c >> 1) & 1, c >> 2));
            double weight = w.x * w.y * w.z;
            if (((open >> c) & 1) && weight > best)
            {
                best = weight;
                home = c;
            }
        }

        // Samples at a cell center give every other corner zero weight, so a dark home cell falls back to its brightest open neighbor
        if (best == 0.0)
        {
            const glm::ivec3 offsets[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
            glm::ivec3 inside           = glm::ivec3(glm::floor(glm::clamp(pos - glm::dvec3(m_snapshotMin), 0.5, kSize - 0.5)));
            glm::dvec3 brightest(0.0);
            for (int i = 0; i < 6; i++)
            {
                glm::ivec3 neighbor = glm::clamp(inside + offsets[i], glm::ivec3(0), glm::ivec3(kSize - 1));
                const uint16_t *texel =
                        &texels[((size_t) ((neighbor.z >> 4) * kSize + neighbor.y) * kSize * 16 + (neighbor.z & 15) * kSize + neighbor.x) * 4];
                glm::dvec3 light = glm::dvec3(glm::unpackHalf1x16(texel[0]), glm::unpackHalf1x16(texel[1]), glm::unpackHalf1x16(texel[2]));
                if (!m_grid->IsSolid(glm::dvec3(m_snapshotMin + neighbor) + 0.5) &&
                    light.x + light.y + light.z > brightest.x + brightest.y + brightest.z)
                {
                    brightest = light;
                }
            }

            return glm::sqrt(glm::max(brightest * 4.0, 0.0));
        }
    }

    uint32_t reach = 1 << home;
    for (int i = 0; i < 3; i++)
    {
        uint32_t spread = 0;
        for (int c = 0; c < 8; c++)
        {
            if ((reach >> c) & 1)
            {
                spread |= (1 << (c ^ 1)) | (1 << (c ^ 2)) | (1 << (c ^ 4));
            }
        }

        reach |= spread & open;
    }

    for (int c = 0; c < 8; c++)
    {
        if (!((reach >> c) & 1))
        {
            corners[c] = corners[home];
        }
    }

    glm::dvec3 x0 = glm::mix(glm::mix(corners[0], corners[1], f.x), glm::mix(corners[2], corners[3], f.x), f.y);
    glm::dvec3 x1 = glm::mix(glm::mix(corners[4], corners[5], f.x), glm::mix(corners[6], corners[7], f.x), f.y);
    return glm::sqrt(glm::max(glm::mix(x0, x1, f.z) * 4.0, 0.0));
}

#endif
