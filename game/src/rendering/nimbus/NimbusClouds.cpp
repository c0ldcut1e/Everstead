#ifndef EVERSTEAD_DEDICATED_SERVER

#include "NimbusClouds.h"

#include <algorithm>
#include <cmath>

#include "../Framebuffer.h"
#include "../GlStateManager.h"
#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Texture.h"
#include "../Textures.h"

#include <glm/gtc/type_ptr.hpp>

NimbusClouds::NimbusClouds(int width, int height, const NimbusSettings *settings)
{
    m_settings    = settings;
    m_framebuffer = new Framebuffer(std::max(width / 2, 1), std::max(height / 2, 1), eTextureFormat_RGBA16F);
    RenderDevice::GetInstance()->SetTextureFilter(m_framebuffer->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    m_shader          = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_clouds.frag");
    m_compositeShader = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_composite.frag");
}

NimbusClouds::~NimbusClouds()
{
    delete m_compositeShader;
    delete m_shader;
    delete m_framebuffer;
}

void NimbusClouds::Resize(int width, int height)
{
    m_framebuffer->Resize(std::max(width / 2, 1), std::max(height / 2, 1));
    RenderDevice::GetInstance()->SetTextureFilter(m_framebuffer->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
}

void NimbusClouds::Render(Framebuffer *outputFramebuffer, Texture *cloudTexture, const glm::dmat4 &invViewProjection, const glm::vec3 &cloudColor,
                          const glm::vec2 &cloudCell, float cloudBottom, const glm::vec4 &flash, float rain, const glm::vec3 &sunDirection,
                          float maxDistance, double time)
{
    RenderDevice *device = RenderDevice::GetInstance();
    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();

    m_framebuffer->Bind();
    device->SetViewport(0, 0, m_framebuffer->GetWidth(), m_framebuffer->GetHeight());
    m_shader->Bind();
    Textures::Bind(outputFramebuffer->GetDepthTexture(), 0);
    Textures::Bind(cloudTexture->GetId(), 1);
    m_shader->SetMat4("u_invViewProj", glm::value_ptr(invViewProjection));
    m_shader->SetVec4("u_flash", flash.x, flash.y, flash.z, flash.w);
    m_shader->SetVec3("u_cloudColor", cloudColor.x, cloudColor.y, cloudColor.z);
    m_shader->SetFloat("u_cloudBottom", cloudBottom);
    m_shader->SetVec3("u_sunDirection", sunDirection.x, sunDirection.y, sunDirection.z);
    m_shader->SetFloat("u_rain", rain);
    m_shader->SetFloat("u_maxDistance", maxDistance);
    m_shader->SetVec2("u_cloudCell", cloudCell.x, cloudCell.y);
    m_shader->SetFloat("u_time", (float) fmod(time, 3600.0));
    m_shader->SetInt("u_steps", m_settings->cloudSteps);
    device->BeginGpuTimer("Nimbus.Clouds");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();

    outputFramebuffer->Bind();
    device->SetViewport(0, 0, outputFramebuffer->GetWidth(), outputFramebuffer->GetHeight());
    eBLEND_FACTOR blendSource;
    eBLEND_FACTOR blendDestination;
    GlStateManager::GetBlendFunc(&blendSource, &blendDestination);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_One, eBlendFactor_SrcAlpha);
    m_compositeShader->Bind();
    Textures::Bind(m_framebuffer->GetColorTexture(), 0);
    device->BeginGpuTimer("Nimbus.CloudComposite");
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->EndGpuTimer();
    GlStateManager::SetBlendFunc(blendSource, blendDestination);
    GlStateManager::DisableBlend();

    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

Shader *NimbusClouds::GetShader() const
{
    return m_shader;
}

#endif
