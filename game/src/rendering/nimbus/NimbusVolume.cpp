#ifndef EVERSTEAD_DEDICATED_SERVER

#include "NimbusVolume.h"

#include <algorithm>
#include <cmath>

#include "../Framebuffer.h"
#include "../GlStateManager.h"
#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Textures.h"

#include <glm/gtc/type_ptr.hpp>

NimbusVolume::NimbusVolume(int width, int height, const NimbusSettings *settings, const NimbusGrid *grid, const NimbusLights *lights,
                           const NimbusFlood *flood, const NimbusSunCache *sunCache, const DynamicLights *dynamicLights)
{
    RenderDevice *device = RenderDevice::GetInstance();
    m_settings           = settings;
    m_grid               = grid;
    m_lights             = lights;
    m_flood              = flood;
    m_sunCache           = sunCache;
    m_dynamicLights      = dynamicLights;

    m_spotShadowFramebuffer = new Framebuffer(2048, 1024, eTextureFormat_RGBA16F);
    device->SetTextureFilter(m_spotShadowFramebuffer->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    m_spotShadowShader = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_spot_shadow.frag");

    // The froxel slices are laid out 8 to a row, so the slice count has to be a multiple of 8
    m_froxelGrid               = glm::ivec3(160, 90, 64);
    m_froxelScatterFramebuffer = new Framebuffer(m_froxelGrid.x * 8, m_froxelGrid.y * m_froxelGrid.z / 8, eTextureFormat_RGBA16F);
    m_froxelFramebuffer        = new Framebuffer(m_froxelGrid.x * 8, m_froxelGrid.y * m_froxelGrid.z / 8, eTextureFormat_RGBA16F);
    device->SetTextureFilter(m_froxelFramebuffer->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    m_volumeShader    = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_volume.frag");
    m_integrateShader = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_froxel_integrate.frag");

    m_volumeFramebuffer = new Framebuffer(std::max(width / 2, 1), std::max(height / 2, 1), eTextureFormat_RGBA16F);
    device->SetTextureFilter(m_volumeFramebuffer->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    m_applyShader     = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_froxel_apply.frag");
    m_compositeShader = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_composite.frag");
}

NimbusVolume::~NimbusVolume()
{
    delete m_compositeShader;
    delete m_applyShader;
    delete m_volumeFramebuffer;
    delete m_integrateShader;
    delete m_volumeShader;
    delete m_froxelFramebuffer;
    delete m_froxelScatterFramebuffer;
    delete m_spotShadowShader;
    delete m_spotShadowFramebuffer;
}

void NimbusVolume::Resize(int width, int height)
{
    m_volumeFramebuffer->Resize(std::max(width / 2, 1), std::max(height / 2, 1));
    RenderDevice::GetInstance()->SetTextureFilter(m_volumeFramebuffer->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
}

void NimbusVolume::Render(Framebuffer *sceneFramebuffer, const glm::dmat4 &invViewProjection, const glm::dvec3 &cameraPos, double time)
{
    if (!m_lights->HasVolumetricLight() && m_dynamicLights->GetFogVolumes().empty())
    {
        return;
    }

    RenderDevice *device = RenderDevice::GetInstance();
    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();

    float fogPositions[DynamicLights::kMaxFogVolumes * 4];
    float fogExtents[DynamicLights::kMaxFogVolumes * 4];
    float fogColors[DynamicLights::kMaxFogVolumes * 4];
    float fogParams[DynamicLights::kMaxFogVolumes * 4];
    int fogCount = (int) m_dynamicLights->GetFogVolumes().size();
    for (int i = 0; i < fogCount; i++)
    {
        const DynamicLights::FogVolume &fog = m_dynamicLights->GetFogVolumes()[(size_t) i];
        fogPositions[i * 4]                 = (float) (fog.position.x - cameraPos.x);
        fogPositions[i * 4 + 1]             = (float) (fog.position.y - cameraPos.y);
        fogPositions[i * 4 + 2]             = (float) (fog.position.z - cameraPos.z);
        fogPositions[i * 4 + 3]             = (float) fog.shape;
        fogExtents[i * 4]                   = fog.extent.x;
        fogExtents[i * 4 + 1]               = fog.extent.y;
        fogExtents[i * 4 + 2]               = fog.extent.z;
        fogExtents[i * 4 + 3]               = fog.density;
        fogColors[i * 4]                    = fog.color.x;
        fogColors[i * 4 + 1]                = fog.color.y;
        fogColors[i * 4 + 2]                = fog.color.z;
        fogColors[i * 4 + 3]                = fog.edge;
        fogParams[i * 4]                    = fog.noise;
        fogParams[i * 4 + 1]                = fog.drift;
        fogParams[i * 4 + 2]                = fog.ambient;
        fogParams[i * 4 + 3]                = 0.0f;
    }

    m_spotShadowFramebuffer->Bind();
    device->SetViewport(0, 0, m_spotShadowFramebuffer->GetWidth(), m_spotShadowFramebuffer->GetHeight());
    m_spotShadowShader->Bind();
    Textures::Bind(m_grid->GetTexture(), 0);
    m_spotShadowShader->SetInt("u_nimbusOcclusion", m_settings->occlusion ? 1 : 0);
    m_lights->Bind(m_spotShadowShader, cameraPos);
    m_grid->Bind(m_spotShadowShader, cameraPos);
    device->BeginGpuTimer("Nimbus.SpotShadow");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();

    m_froxelScatterFramebuffer->Bind();
    device->SetViewport(0, 0, m_froxelScatterFramebuffer->GetWidth(), m_froxelScatterFramebuffer->GetHeight());
    m_volumeShader->Bind();
    Textures::Bind(m_flood->GetTexture(), 0);
    Textures::Bind(m_sunCache->GetTexture(), 1);
    Textures::Bind(m_grid->GetTexture(), 2);
    m_volumeShader->SetMat4("u_invViewProj", glm::value_ptr(invViewProjection));
    m_volumeShader->SetVec3("u_cameraPos", (float) cameraPos.x, (float) cameraPos.y, (float) cameraPos.z);
    m_volumeShader->SetFloat("u_time", (float) fmod(time, 3600.0));
    m_volumeShader->SetFloat("u_maxDistance", m_settings->volumeDistance);
    m_volumeShader->SetVec3("u_froxelGrid", (float) m_froxelGrid.x, (float) m_froxelGrid.y, (float) m_froxelGrid.z);
    m_volumeShader->SetFloat("u_airDensity", m_settings->airDensity);
    m_volumeShader->SetFloat("u_anisotropy", m_settings->anisotropy);
    m_volumeShader->SetInt("u_nimbusOcclusion", m_settings->occlusion ? 1 : 0);
    m_volumeShader->SetInt("u_fogCount", fogCount);
    m_volumeShader->SetVec4Array("u_fogPositions", fogCount, fogPositions);
    m_volumeShader->SetVec4Array("u_fogExtents", fogCount, fogExtents);
    m_volumeShader->SetVec4Array("u_fogColors", fogCount, fogColors);
    m_volumeShader->SetVec4Array("u_fogParams", fogCount, fogParams);
    m_lights->Bind(m_volumeShader, cameraPos);
    m_grid->Bind(m_volumeShader, cameraPos);
    glm::dvec3 floodOrigin = glm::dvec3(m_flood->GetMin()) - cameraPos;
    m_volumeShader->SetVec3("u_nimbusFloodOrigin", (float) floodOrigin.x, (float) floodOrigin.y, (float) floodOrigin.z);
    device->BeginGpuTimer("Nimbus.Scatter");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();

    m_froxelFramebuffer->Bind();
    m_integrateShader->Bind();
    Textures::Bind(m_froxelScatterFramebuffer->GetColorTexture(), 0);
    m_integrateShader->SetFloat("u_maxDistance", m_settings->volumeDistance);
    m_integrateShader->SetVec3("u_froxelGrid", (float) m_froxelGrid.x, (float) m_froxelGrid.y, (float) m_froxelGrid.z);
    device->BeginGpuTimer("Nimbus.Integrate");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();

    m_volumeFramebuffer->Bind();
    device->SetViewport(0, 0, m_volumeFramebuffer->GetWidth(), m_volumeFramebuffer->GetHeight());
    m_applyShader->Bind();
    Textures::Bind(sceneFramebuffer->GetDepthTexture(), 0);
    Textures::Bind(m_froxelFramebuffer->GetColorTexture(), 1);
    Textures::Bind(m_flood->GetTexture(), 2);
    Textures::Bind(m_spotShadowFramebuffer->GetColorTexture(), 3);
    m_applyShader->SetMat4("u_invViewProj", glm::value_ptr(invViewProjection));
    m_applyShader->SetFloat("u_maxDistance", m_settings->volumeDistance);
    m_applyShader->SetVec3("u_froxelGrid", (float) m_froxelGrid.x, (float) m_froxelGrid.y, (float) m_froxelGrid.z);
    constexpr int kPointSteps[] = {8, 12, 16, 24};
    m_applyShader->SetInt("u_pointSteps", kPointSteps[m_settings->volumeQuality]);
    m_applyShader->SetFloat("u_airDensity", m_settings->airDensity);
    m_applyShader->SetFloat("u_anisotropy", m_settings->anisotropy);
    m_applyShader->SetInt("u_nimbusOcclusion", m_settings->occlusion ? 1 : 0);
    m_applyShader->SetVec3("u_nimbusFloodOrigin", (float) floodOrigin.x, (float) floodOrigin.y, (float) floodOrigin.z);
    m_applyShader->SetInt("u_nimbusFloodScatter", m_settings->blockLighting && m_settings->volumetricLight ? 1 : 0);
    m_lights->Bind(m_applyShader, cameraPos);
    device->BeginGpuTimer("Nimbus.Apply");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();

    sceneFramebuffer->Bind();
    device->SetViewport(0, 0, sceneFramebuffer->GetWidth(), sceneFramebuffer->GetHeight());
    eBLEND_FACTOR blendSource;
    eBLEND_FACTOR blendDestination;
    GlStateManager::GetBlendFunc(&blendSource, &blendDestination);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_One, eBlendFactor_SrcAlpha);
    m_compositeShader->Bind();
    Textures::Bind(m_volumeFramebuffer->GetColorTexture(), 0);
    device->BeginGpuTimer("Nimbus.VolumeComposite");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();
    GlStateManager::SetBlendFunc(blendSource, blendDestination);
    GlStateManager::DisableBlend();

    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

Shader *NimbusVolume::GetVolumeShader() const
{
    return m_volumeShader;
}

Shader *NimbusVolume::GetApplyShader() const
{
    return m_applyShader;
}

#endif
