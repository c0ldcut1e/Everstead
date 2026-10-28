#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Nimbus.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../core/Everstead.h"
#include "../../core/FrameProfiler.h"
#include "../../entity/player/MultiPlayerLocalPlayer.h"
#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/AtmosphereC2SPacket.h"
#include "../../network/packet/c2s/DynamicLightC2SPacket.h"
#include "../../network/packet/c2s/FogVolumeC2SPacket.h"
#include "../../scene/Camera.h"
#include "../../ui/imgui/imgui.h"
#include "../../utils/Time.h"
#include "../../world/Level.h"
#include "../../world/lighting/LightSource.h"
#include "../Framebuffer.h"
#include "../GlStateManager.h"
#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Textures.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vector_relational.hpp>

Nimbus::Nimbus(int width, int height, DynamicLights *dynamicLights, const ItemLights *itemLights)
{
    m_settings.enabled          = RenderDevice::GetInstance()->GetBackend() != eRenderBackend_OpenGL;
    m_settings.blockLighting    = false;
    m_settings.clouds           = false;
    m_settings.fog              = false;
    m_settings.surfaceLights    = true;
    m_settings.volumetricLight  = true;
    m_settings.occlusion        = true;
    m_settings.lightingMode     = eNimbusLightingMode_Floodfill;
    m_settings.quality          = -1;
    m_settings.shadowSamples    = 16;
    m_settings.bounce           = 0.35f;
    m_settings.volumeQuality    = 2;
    m_settings.volumeDistance   = 96.0f;
    m_settings.sectionsPerFrame = 32;
    m_settings.floodIterations  = 1;
    m_settings.sunSlotsPerFrame = 24;
    m_settings.cloudSteps       = 24;
    m_settings.atmosphere       = 1.0f;
    m_settings.airDensity       = dynamicLights->GetAirDensity();
    m_settings.anisotropy       = dynamicLights->GetAnisotropy();

    m_dynamicLights   = dynamicLights;
    m_selectedLightId = 0;
    m_selectedFogId   = 0;
    m_width           = width;
    m_height          = height;
    m_time            = 0.0;

    m_grid     = new NimbusGrid();
    m_lights   = new NimbusLights(width, height, &m_settings, dynamicLights);
    m_sunCache = new NimbusSunCache(&m_settings, m_grid, dynamicLights);
    m_flood    = new NimbusFlood(&m_settings, m_grid, m_lights, dynamicLights, itemLights);
    m_volume   = new NimbusVolume(width, height, &m_settings, m_grid, m_lights, m_flood, m_sunCache, dynamicLights);
    m_clouds   = new NimbusClouds(width, height, &m_settings);
}

Nimbus::~Nimbus()
{
    delete m_clouds;
    delete m_volume;
    delete m_flood;
    delete m_sunCache;
    delete m_lights;
    delete m_grid;
}

void Nimbus::Resize(int width, int height)
{
    m_width  = width;
    m_height = height;
    m_lights->Resize(width, height);
    m_volume->Resize(width, height);
    m_clouds->Resize(width, height);
}

void Nimbus::Update(Level *level, const glm::dvec3 &cameraPos, const glm::dmat4 &viewProjection)
{
    FrameProfilerScope scope("Nimbus.Update");
    m_time += Time::GetDelta();
    const Options &options = Everstead::GetInstance()->options;
    bool blockLighting     = options.nimbusEnabled && options.nimbusLighting;
    m_settings.clouds      = options.nimbusEnabled && options.nimbusClouds;
    m_settings.fog         = options.nimbusEnabled && options.nimbusFog;
    if (blockLighting && !m_settings.blockLighting)
    {
        m_flood->MarkEmissionDirty();
    }

    m_settings.blockLighting = blockLighting;
    int quality              = options.nimbusQuality;
    if (quality != m_settings.quality)
    {
        constexpr int kShadowSamples[]    = {8, 16, 32};
        constexpr int kVolumeQuality[]    = {0, 2, 3};
        constexpr float kVolumeDistance[] = {64.0f, 96.0f, 128.0f};
        constexpr int kFloodIterations[]  = {1, 1, 2};
        constexpr int kCloudSteps[]       = {16, 24, 40};
        m_settings.quality                = quality;
        m_settings.volumeQuality          = kVolumeQuality[quality];
        m_settings.volumeDistance         = kVolumeDistance[quality];
        m_settings.floodIterations        = kFloodIterations[quality];
        m_settings.cloudSteps             = kCloudSteps[quality];
        if (m_settings.shadowSamples != kShadowSamples[quality])
        {
            m_settings.shadowSamples = kShadowSamples[quality];
            m_sunCache->Invalidate();
        }
    }

    // We thicken the air in rain, storms and around sunrise and sunset on top of the synced base values
    float sunHeight       = (float) level->GetWorldProvider().GetSunDirection().y;
    float twilight        = std::clamp(1.0f - fabsf(sunHeight) / 0.3f, 0.0f, 1.0f);
    float night           = std::clamp(-sunHeight * 2.0f, 0.0f, 1.0f);
    float rain            = level->GetRainLevel(1.0f);
    float thunder         = level->GetThunderLevel(1.0f);
    m_settings.atmosphere = 1.0f + rain * 1.5f + thunder + twilight * twilight * 0.8f + night * 0.3f;
    m_settings.airDensity = m_dynamicLights->GetAirDensity() * m_settings.atmosphere;
    m_settings.anisotropy = m_dynamicLights->GetAnisotropy() * (1.0f - rain * 0.4f);

    if (!m_settings.enabled || (!m_settings.blockLighting && m_dynamicLights->GetLights().empty() && m_dynamicLights->GetFogVolumes().empty()))
    {
        m_flood->Clear();
        return;
    }

    m_lights->Cull(cameraPos, viewProjection);
    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    bool drew = false;
    if (m_settings.occlusion || m_settings.blockLighting)
    {
        m_grid->Update(level, cameraPos, m_settings.sectionsPerFrame, m_settings.blockLighting);
        FrameProfilerScope sunScope("Nimbus.SunCache");
        RenderDevice::GetInstance()->BeginGpuTimer("Nimbus.SunCache");
        drew = m_sunCache->Update(cameraPos);
        RenderDevice::GetInstance()->EndGpuTimer();
    }

    {
        FrameProfilerScope floodScope("Nimbus.Flood");
        RenderDevice::GetInstance()->BeginGpuTimer("Nimbus.Flood");
        drew = m_flood->Render(cameraPos) || drew;
        RenderDevice::GetInstance()->EndGpuTimer();
    }

    if (drew)
    {
        RenderDevice::GetInstance()->BindFramebuffer({});
        RenderDevice::GetInstance()->SetViewport(0, 0, m_width, m_height);
    }

    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void Nimbus::MarkSectionDirty(const SectionPos &pos)
{
    m_grid->MarkSectionDirty(pos);
}

void Nimbus::BindSurface(Shader *shader, const glm::dvec3 &cameraPos) const
{
    FrameProfilerScope scope("Nimbus.BindSurface");
    bool active = m_settings.enabled && m_settings.surfaceLights && (m_settings.blockLighting || !m_dynamicLights->GetLights().empty());
    shader->SetInt("u_nimbusSurface", active ? 1 : 0);
    if (active)
    {
        m_lights->Bind(shader, cameraPos);
        m_grid->Bind(shader, cameraPos);
        glm::dvec3 floodOrigin = glm::dvec3(m_flood->GetMin()) - cameraPos;
        shader->SetVec3("u_nimbusFloodOrigin", (float) floodOrigin.x, (float) floodOrigin.y, (float) floodOrigin.z);
        shader->SetInt("u_nimbusFloodLights", m_flood->GetLightCount());
        shader->SetInt("u_nimbusOcclusion", m_settings.occlusion ? 1 : 0);
        shader->SetInt("u_nimbusMode", m_settings.lightingMode);
        shader->SetFloat("u_nimbusBounce", m_settings.bounce);
        shader->SetFloat("u_nimbusTilePixels", (float) NimbusLights::kTileSize);
    }

    Textures::Bind(m_lights->GetTileTexture(), 4);
    Textures::Bind(m_flood->GetTexture(), 5);
    Textures::Bind(m_grid->GetTexture(), 6);
    Textures::Bind(m_sunCache->GetTexture(), 7);
}

void Nimbus::RenderVolume(Framebuffer *sceneFramebuffer, const glm::dmat4 &invViewProjection, const glm::dvec3 &cameraPos)
{
    if (!m_settings.enabled)
    {
        return;
    }

    m_volume->Render(sceneFramebuffer, invViewProjection, cameraPos, m_time);
}

void Nimbus::RenderClouds(Framebuffer *outputFramebuffer, Texture *cloudTexture, const glm::dmat4 &invViewProjection, const glm::vec3 &cloudColor,
                          const glm::vec2 &cloudCell, float cloudBottom, const glm::vec4 &flash, float rain, const glm::vec3 &sunDirection,
                          float maxDistance)
{
    FrameProfilerScope scope("Nimbus.Clouds");
    m_clouds->Render(outputFramebuffer, cloudTexture, invViewProjection, cloudColor, cloudCell, cloudBottom, flash, rain, sunDirection, maxDistance,
                     m_time);
}

Shader *Nimbus::GetVolumeShader() const
{
    return m_volume->GetVolumeShader();
}

Shader *Nimbus::GetApplyShader() const
{
    return m_volume->GetApplyShader();
}

Shader *Nimbus::GetCloudShader() const
{
    return m_clouds->GetShader();
}

glm::dvec3 Nimbus::Shoulder(const glm::dvec3 &light) const
{
    double peak = std::max({light.x, light.y, light.z});
    if (peak <= 0.7)
    {
        return light;
    }

    return light * ((0.7 + 0.3 * (1.0 - exp((0.7 - peak) / 0.3))) / peak);
}

// This mirrors NimbusSurfaceLight in nimbus_surface.glsl for a point that faces every light
glm::dvec3 Nimbus::SampleLight(const glm::dvec3 &pos, const glm::dvec3 &baseLight) const
{
    FrameProfilerScope scope("Nimbus.SampleLight");
    glm::dvec3 total(0.0);
    if (!m_settings.enabled || !m_settings.surfaceLights || (!m_settings.blockLighting && m_dynamicLights->GetLights().empty()))
    {
        return total;
    }

    bool floodfill = m_settings.lightingMode == eNimbusLightingMode_Floodfill;
    double fade    = m_flood->GetFade(pos);
    if (m_flood->GetLightCount() > 0 && fade < 1.0)
    {
        total = m_flood->SampleSnapshot(pos) * ((1.0 - fade) * (floodfill ? 1.0 : (double) m_settings.bounce));
        if (m_settings.blockLighting)
        {
            total        = Shoulder(total);
            double peak  = std::max({total.x, total.y, total.z});
            double white = 1.0 - glm::smoothstep(0.35, 0.6, (peak - std::min({total.x, total.y, total.z})) / std::max(peak, 0.0001));
            double level = std::clamp(peak, 0.0, 1.0);
            double ramp  = level / ((1.0 - level) * 3.0 + 1.0);
            total.y      = glm::mix(total.y, total.y * ((ramp * 0.6 + 0.4) * 0.6 + 0.4), white);
            total.z      = glm::mix(total.z, total.z * (ramp * ramp * 0.6 + 0.4), white);
            total        = glm::max(total - glm::clamp(baseLight, 0.0, 1.0), total * 0.15) *
                           (double) LightSource::SampleBlockLightScale(LightSource::s_blockFlicker);
        }
    }

    int suns     = 0;
    double decay = m_lights->FloodDecay();
    for (size_t i = 0; i < m_dynamicLights->GetLights().size(); i++)
    {
        const DynamicLights::Light &light = m_dynamicLights->GetLights()[i];
        glm::dvec3 color                  = glm::dvec3(light.color) * (double) light.intensity;
        glm::dvec3 lightPos               = light.position;
        double attenuation                = 1.0;
        double softness                   = light.size;
        bool occluded                     = m_settings.occlusion && light.occlusion;
        if (light.type == DynamicLights::eLightType_Directional)
        {
            lightPos = pos - glm::normalize(glm::dvec3(light.direction)) * 256.0;
            softness *= 256.0;
            if (occluded)
            {
                occluded = suns < DynamicLights::kMaxSuns;
                suns++;
            }
        }
        else
        {
            double distance = glm::length(lightPos - pos);
            double clamped  = std::max(distance, 1.0);
            double window   = std::clamp(1.0 - pow(distance / std::max((double) light.radius, 0.1), 4.0), 0.0, 1.0);
            attenuation     = sqrt(exp(-decay * (clamped - 1.0)) / clamped) * window * window;
            if (light.type == DynamicLights::eLightType_Spot)
            {
                double coneCos = cos(light.coneAngle * 0.5 * std::numbers::pi / 180.0);
                double facing  = glm::dot(glm::normalize(pos - lightPos), glm::normalize(glm::dvec3(light.direction)));
                double t       = std::clamp((facing - coneCos) / std::max((1.0 - coneCos) * 0.2, 0.0001), 0.0, 1.0);
                attenuation *= t * t * (3.0 - 2.0 * t);
            }
        }

        if (floodfill && light.type == DynamicLights::eLightType_Point && occluded)
        {
            total += color * attenuation * fade;
            continue;
        }

        if (attenuation > 0.001)
        {
            total += color * attenuation * (occluded ? m_grid->SoftTrace(pos, lightPos, softness) : 1.0);
        }
    }

    return m_settings.blockLighting ? Shoulder(total) : total;
}

void Nimbus::RenderImGui(const glm::dvec3 &cameraPos)
{
    FrameProfilerScope scope("Nimbus.ImGui");
    if (!ImGui::Begin("Nimbus"))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Enabled", &m_settings.enabled);
    ImGui::Checkbox("Surface Lights", &m_settings.surfaceLights);
    ImGui::SameLine();
    ImGui::Checkbox("Volumetric Light", &m_settings.volumetricLight);
    if (ImGui::Checkbox("Block Occlusion", &m_settings.occlusion))
    {
        m_sunCache->Invalidate();
    }

    static const char *const kModeNames[] = {"Floodfill", "Sharp"};
    int mode                              = (int) m_settings.lightingMode;
    if (ImGui::Combo("Lighting Mode", &mode, kModeNames, 2))
    {
        m_settings.lightingMode = (eNIMBUS_LIGHTING_MODE) mode;
    }

    ImGui::Text("Sections built %d, sun slots traced %d", (int) m_grid->GetRebuiltSlots().size(), m_sunCache->GetSlotsBuilt());

    if (ImGui::CollapsingHeader("Quality"))
    {
        if (ImGui::SliderInt("Sun Cache Samples", &m_settings.shadowSamples, 1, 64))
        {
            m_sunCache->Invalidate();
        }

        ImGui::SliderFloat("Flood Bounce", &m_settings.bounce, 0.0f, 1.0f);
        ImGui::SliderInt("Volume Quality", &m_settings.volumeQuality, 0, 3);

        ImGui::SliderFloat("Volume Distance", &m_settings.volumeDistance, 16.0f, 256.0f);
        ImGui::SliderInt("Sections Per Frame", &m_settings.sectionsPerFrame, 1, NimbusGrid::kSlotCount);
        ImGui::SliderInt("Flood Iterations", &m_settings.floodIterations, 1, 16);
        ImGui::SliderInt("Cloud Steps", &m_settings.cloudSteps, 4, 64);
        ImGui::SliderInt("Sun Slots Per Frame", &m_settings.sunSlotsPerFrame, 1, NimbusGrid::kSlotCount);
    }

    ClientConnection *connection = Everstead::GetInstance()->GetLocalPlayer()->GetClientConnection();
    if (ImGui::CollapsingHeader("Atmosphere", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float airDensity = m_dynamicLights->GetAirDensity();
        float anisotropy = m_dynamicLights->GetAnisotropy();
        bool changed     = ImGui::SliderFloat("Air Density", &airDensity, 0.0f, 0.05f, "%.4f");
        changed |= ImGui::SliderFloat("Anisotropy", &anisotropy, -0.9f, 0.9f);
        if (changed)
        {
            m_dynamicLights->SetAtmosphere(airDensity, anisotropy);
            connection->Send(std::shared_ptr<Packet>(new AtmosphereC2SPacket(airDensity, anisotropy)));
        }
    }

    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const Camera *camera                            = Everstead::GetInstance()->GetCamera();
        const std::vector<DynamicLights::Light> &lights = m_dynamicLights->GetLights();
        DynamicLights::Light added;
        added.id = 0;
        if (ImGui::Button("Add Point"))
        {
            added = {1,
                     DynamicLights::eLightType_Point,
                     cameraPos,
                     glm::vec3(0.0f, -1.0f, 0.0f),
                     glm::vec3(1.0f, 0.75f, 0.45f),
                     1.0f,
                     12.0f,
                     45.0f,
                     0.3f,
                     false,
                     true};
        }

        ImGui::SameLine();
        if (ImGui::Button("Add Spot"))
        {
            added = {1,
                     DynamicLights::eLightType_Spot,
                     cameraPos,
                     glm::vec3(camera->GetFront()),
                     glm::vec3(1.0f, 1.0f, 1.0f),
                     1.5f,
                     24.0f,
                     40.0f,
                     0.3f,
                     false,
                     true};
        }

        ImGui::SameLine();
        if (ImGui::Button("Add Directional"))
        {
            added = {1,
                     DynamicLights::eLightType_Directional,
                     cameraPos,
                     glm::vec3(camera->GetFront()),
                     glm::vec3(0.6f, 0.7f, 1.0f),
                     0.4f,
                     0.0f,
                     0.0f,
                     0.02f,
                     false,
                     true};
        }

        if (added.id != 0)
        {
            do
            {
                added.id = m_random.NextUInt();
            } while (added.id == 0 || m_dynamicLights->GetLight(added.id));

            if (m_dynamicLights->SetLight(added))
            {
                connection->Send(std::shared_ptr<Packet>(new DynamicLightC2SPacket(added)));
                m_selectedLightId = added.id;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            while (!lights.empty())
            {
                connection->Send(std::shared_ptr<Packet>(new DynamicLightC2SPacket(lights[0].id)));
                m_dynamicLights->RemoveLight(lights[0].id);
            }
        }

        static const char *const kTypeNames[] = {"Point", "Spot", "Directional"};
        for (size_t i = 0; i < lights.size(); i++)
        {
            char label[48];
            snprintf(label, sizeof(label), "%s %zu", kTypeNames[lights[i].type], i);
            if (ImGui::Selectable(label, m_selectedLightId == lights[i].id))
            {
                m_selectedLightId = lights[i].id;
            }
        }

        const DynamicLights::Light *selected = m_dynamicLights->GetLight(m_selectedLightId);
        if (selected)
        {
            DynamicLights::Light light = *selected;
            ImGui::Separator();
            int type     = (int) light.type;
            bool changed = ImGui::Combo("Type", &type, kTypeNames, 3);
            light.type   = (DynamicLights::eLIGHT_TYPE) type;
            changed |= ImGui::DragScalarN("Position", ImGuiDataType_Double, glm::value_ptr(light.position), 3, 0.1f);
            changed |= ImGui::SliderFloat3("Direction", glm::value_ptr(light.direction), -1.0f, 1.0f);
            changed |= ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
            changed |= ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 8.0f);
            changed |= ImGui::SliderFloat("Radius", &light.radius, 1.0f, 64.0f);
            changed |= ImGui::SliderFloat("Cone Angle", &light.coneAngle, 1.0f, 179.0f);
            // A directional light's size is the tangent of its angular radius, so small values already give wide penumbras
            changed |= ImGui::SliderFloat("Size", &light.size, 0.0f, light.type == DynamicLights::eLightType_Directional ? 0.1f : 4.0f, "%.3f");
            changed |= ImGui::Checkbox("Volumetric", &light.volumetric);
            changed |= ImGui::Checkbox("Block Occlusion", &light.occlusion);
            if (ImGui::Button("Move To Camera"))
            {
                light.position  = cameraPos;
                light.direction = glm::vec3(camera->GetFront());
                changed         = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                connection->Send(std::shared_ptr<Packet>(new DynamicLightC2SPacket(light.id)));
                m_dynamicLights->RemoveLight(light.id);
            }
            else if (changed && m_dynamicLights->SetLight(light))
            {
                connection->Send(std::shared_ptr<Packet>(new DynamicLightC2SPacket(light)));
            }
        }
    }

    if (ImGui::CollapsingHeader("Fog Volumes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const std::vector<DynamicLights::FogVolume> &fogVolumes = m_dynamicLights->GetFogVolumes();
        DynamicLights::FogVolume added;
        added.id = 0;
        if (ImGui::Button("Add Box Fog"))
        {
            added = {
                    1,    DynamicLights::eFogShape_Box, cameraPos, glm::vec3(8.0f, 4.0f, 8.0f), glm::vec3(0.8f, 0.85f, 0.9f), 0.15f, 0.3f, 0.6f, 0.5f,
                    0.05f};
        }

        ImGui::SameLine();
        if (ImGui::Button("Add Sphere Fog"))
        {
            added = {1, DynamicLights::eFogShape_Sphere, cameraPos, glm::vec3(6.0f), glm::vec3(0.8f, 0.85f, 0.9f), 0.15f, 0.5f, 0.6f, 0.5f, 0.05f};
        }

        if (added.id != 0)
        {
            do
            {
                added.id = m_random.NextUInt();
            } while (added.id == 0 || m_dynamicLights->GetFogVolume(added.id));

            if (m_dynamicLights->SetFogVolume(added))
            {
                connection->Send(std::shared_ptr<Packet>(new FogVolumeC2SPacket(added)));
                m_selectedFogId = added.id;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Fog"))
        {
            while (!fogVolumes.empty())
            {
                connection->Send(std::shared_ptr<Packet>(new FogVolumeC2SPacket(fogVolumes[0].id)));
                m_dynamicLights->RemoveFogVolume(fogVolumes[0].id);
            }
        }

        static const char *const kShapeNames[] = {"Box", "Sphere"};
        for (size_t i = 0; i < fogVolumes.size(); i++)
        {
            char label[48];
            snprintf(label, sizeof(label), "%s Fog %zu", kShapeNames[fogVolumes[i].shape], i);
            if (ImGui::Selectable(label, m_selectedFogId == fogVolumes[i].id))
            {
                m_selectedFogId = fogVolumes[i].id;
            }
        }

        const DynamicLights::FogVolume *selected = m_dynamicLights->GetFogVolume(m_selectedFogId);
        if (selected)
        {
            DynamicLights::FogVolume fog = *selected;
            ImGui::Separator();
            int shape    = (int) fog.shape;
            bool changed = ImGui::Combo("Shape", &shape, kShapeNames, 2);
            fog.shape    = (DynamicLights::eFOG_SHAPE) shape;
            changed |= ImGui::DragScalarN("Fog Position", ImGuiDataType_Double, glm::value_ptr(fog.position), 3, 0.1f);
            changed |= ImGui::SliderFloat3("Extent", glm::value_ptr(fog.extent), 0.5f, 64.0f);
            changed |= ImGui::ColorEdit3("Fog Color", glm::value_ptr(fog.color));
            changed |= ImGui::SliderFloat("Density", &fog.density, 0.0f, 1.0f, "%.3f");
            changed |= ImGui::SliderFloat("Edge Falloff", &fog.edge, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Noise", &fog.noise, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Drift", &fog.drift, 0.0f, 4.0f);
            changed |= ImGui::SliderFloat("Ambient", &fog.ambient, 0.0f, 2.0f);
            if (ImGui::Button("Move Fog To Camera"))
            {
                fog.position = cameraPos;
                changed      = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("Remove Fog"))
            {
                connection->Send(std::shared_ptr<Packet>(new FogVolumeC2SPacket(fog.id)));
                m_dynamicLights->RemoveFogVolume(fog.id);
            }
            else if (changed && m_dynamicLights->SetFogVolume(fog))
            {
                connection->Send(std::shared_ptr<Packet>(new FogVolumeC2SPacket(fog)));
            }
        }
    }

    ImGui::End();
}

bool Nimbus::IsEnabled() const
{
    return m_settings.enabled;
}

bool Nimbus::IsBlockLighting() const
{
    return m_settings.enabled && m_settings.blockLighting;
}

bool Nimbus::HasClouds() const
{
    return m_settings.enabled && m_settings.clouds;
}

bool Nimbus::HasFog() const
{
    return m_settings.enabled && m_settings.fog;
}

float Nimbus::GetAtmosphere() const
{
    return m_settings.atmosphere;
}

double Nimbus::GetFloodFade(const glm::dvec3 &pos) const
{
    return m_flood->GetFade(pos);
}

#endif
