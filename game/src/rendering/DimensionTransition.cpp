#ifndef EVERSTEAD_DEDICATED_SERVER

#include "DimensionTransition.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../core/Everstead.h"
#include "../entity/player/LocalPlayer.h"
#include "../scene/Camera.h"
#include "../sound/SoundEngine.h"
#include "../utils/Time.h"
#include "Framebuffer.h"
#include "GameRenderer.h"
#include "GlStateManager.h"
#include "RenderDevice.h"
#include "Shader.h"
#include "Textures.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

DimensionTransition::DimensionTransition()
{
    m_shader          = new Shader("assets/shader/gamma.vert", "assets/shader/dimension_transition.frag");
    m_compositeShader = new Shader("assets/shader/gamma.vert", "assets/shader/gamma.frag");
    m_target          = new Framebuffer(1, 1);
    m_vao             = RenderDevice::GetInstance()->CreateVertexArray();
    m_active          = false;
    m_finished        = false;
    m_stability       = 1.0f;
    m_startFront      = glm::dvec3(0.0, 0.0, 1.0);
    m_fromTear        = false;
    m_tearSeed        = 0.0f;
    m_tearTravel      = 0.0;
    m_tearOrigin      = glm::dvec3(0.0);
    m_tearAngle       = 0.0;
    m_lastCamera      = glm::dvec3(0.0);
}

DimensionTransition::~DimensionTransition()
{
    RenderDevice::GetInstance()->DeleteVertexArray(m_vao);
    delete m_target;
    delete m_compositeShader;
    delete m_shader;
}

void DimensionTransition::Start()
{
    if (!m_active || m_finished)
    {
        m_startTime  = std::chrono::steady_clock::now();
        float yaw    = Everstead::GetInstance()->GetLocalPlayer()->GetYaw() * (float) std::numbers::pi / 180.0f;
        m_startFront = glm::dvec3(cos(yaw), 0.0, sin(yaw));
        m_fromTear   = false;
        Everstead::GetInstance()->GetSoundEngine()->GetDimensionTransitionSound()->Start();
    }

    m_active   = true;
    m_finished = false;
}

void DimensionTransition::Finish()
{
    if (m_active && !m_finished)
    {
        m_finished   = true;
        m_finishTime = std::max(std::chrono::steady_clock::now(), m_startTime + std::chrono::milliseconds(3500));
        Everstead::GetInstance()->GetSoundEngine()->GetDimensionTransitionSound()->Finish(
                std::chrono::duration<double>(m_finishTime - m_startTime).count());
    }
}

void DimensionTransition::Stop()
{
    m_active = false;
    Everstead::GetInstance()->GetSoundEngine()->GetDimensionTransitionSound()->Stop();
}

bool DimensionTransition::IsCovered() const
{
    return m_active && !m_finished && std::chrono::steady_clock::now() - m_startTime >= std::chrono::milliseconds(m_fromTear ? 200 : 900);
}

bool DimensionTransition::IsTraveling() const
{
    return m_active && !m_finished;
}

double DimensionTransition::GetElapsedSeconds() const
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_startTime).count();
}

// Stability goes from 0 for a fully broken transition to 1 for a clean one
void DimensionTransition::SetStability(float stability)
{
    m_stability = stability;
    Everstead::GetInstance()->GetSoundEngine()->GetDimensionTransitionSound()->SetStability(stability);
}

// We pick up the void exactly where the tear shader draws it, so stepping into the crack shows no seam
void DimensionTransition::SetTear(const glm::dvec3 &center, float seed)
{
    double time  = (double) (Time::GetMilliseconds() % 600000) / 1000.0;
    m_fromTear   = true;
    m_tearSeed   = seed;
    m_tearAngle  = sin(time * 0.2 + seed) * 0.3 + seed;
    m_tearTravel = fmod(time * 6.0 + seed * 5.0, 512.0);
    m_lastCamera = center;
    m_tearOrigin = glm::dvec3(0.0, 0.0, -15.0);
    MoveTearOrigin();
}

// We move through the void as the camera moves, but skip the jump when the server teleports us into the new dimension
void DimensionTransition::MoveTearOrigin()
{
    glm::dvec3 camera = Everstead::GetInstance()->GetCamera()->GetPosition();
    glm::dvec3 delta  = camera - m_lastCamera;
    m_lastCamera      = camera;
    if (glm::length(delta) < 4.0)
    {
        m_tearOrigin += glm::dvec3(cos(m_tearAngle) * delta.x + sin(m_tearAngle) * delta.z, delta.y,
                                   -sin(m_tearAngle) * delta.x + cos(m_tearAngle) * delta.z) *
                        6.0;
    }
}

void DimensionTransition::Render(int width, int height)
{
    if (!m_active)
    {
        return;
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    double elapsed                            = std::chrono::duration<double>(now - m_startTime).count();
    double close                              = 0.0;
    if (m_finished)
    {
        close = std::chrono::duration<double>(now - m_finishTime).count();
        if (close >= 1.0)
        {
            m_active = false;
            return;
        }
    }

    // Speed eases from 3 toward 90 units per second, which stays below the rate where the cube rows start to strobe
    double ramp   = 1.0 - exp(-elapsed / 2.5);
    double travel = 90.0 * elapsed - 87.0 * 2.5 * ramp;
    if (m_fromTear)
    {
        travel = m_tearTravel + 90.0 * elapsed - 84.0 * 2.5 * ramp;
    }

    bool depthTest = GlStateManager::IsDepthTestEnabled();
    bool cull      = GlStateManager::IsCullEnabled();
    bool blend     = GlStateManager::IsBlendEnabled();
    GlStateManager::DisableDepthTest();
    GlStateManager::DisableCull();
    GlStateManager::DisableBlend();
    eBLEND_FACTOR blendSrc;
    eBLEND_FACTOR blendDst;
    GlStateManager::GetBlendFunc(&blendSrc, &blendDst);

    // The raymarch is too heavy for integrated GPUs at full resolution, so we trace at half size and upscale
    RenderDevice *device = RenderDevice::GetInstance();
    m_target->Resize(std::max(width / 2, 1), std::max(height / 2, 1));
    m_target->Bind();
    device->SetViewport(0, 0, m_target->GetWidth(), m_target->GetHeight());

    // The tunnel stays fixed to the direction we faced when it started, so looking around turns the view inside it
    glm::dmat4 cameraRotation =
            glm::inverse(glm::lookAt(glm::dvec3(0.0), Everstead::GetInstance()->GetCamera()->GetFront(), glm::dvec3(0.0, 1.0, 0.0)));
    glm::dmat4 view = glm::lookAt(glm::dvec3(0.0), m_startFront, glm::dvec3(0.0, 1.0, 0.0)) * cameraRotation;
    glm::dvec3 origin(0.0, 0.0, -15.0);
    float focal     = 1.0f;
    float voidBlend = 1.0f;
    float fade      = 1.0f;
    if (m_fromTear)
    {
        // The shader flips x and z back after this, so we flip them here to hand it the spun world ray the tear uses
        glm::dmat4 spin(1.0);
        spin[0][0] = cos(m_tearSeed);
        spin[0][2] = -sin(m_tearSeed);
        spin[2][0] = sin(m_tearSeed);
        spin[2][2] = cos(m_tearSeed);
        view       = glm::scale(glm::dmat4(1.0), glm::dvec3(-1.0, 1.0, -1.0)) * spin * cameraRotation;
        MoveTearOrigin();
        origin    = m_tearOrigin;
        focal     = (float) Everstead::GetInstance()->GetGameRenderer()->GetProjection()[1][1] * 0.5f;
        voidBlend = (float) glm::smoothstep(0.3, 2.5, elapsed);
        fade      = (float) std::min(elapsed / 0.2, 1.0);
    }

    m_shader->Bind();
    m_shader->SetMat4("u_view", glm::value_ptr(view));
    m_shader->SetVec4("u_origin", (float) origin.x, (float) origin.y, (float) origin.z, focal);
    m_shader->SetFloat("u_twist", m_fromTear ? 1.0f : (float) ramp);
    m_shader->SetFloat("u_blend", voidBlend);
    m_shader->SetFloat("u_fade", fade);
    m_shader->SetFloat("u_aspect", (float) width / (float) height);
    m_shader->SetFloat("u_travel", (float) fmod(travel, 512.0));
    m_shader->SetFloat("u_warp", (float) ramp);
    m_shader->SetFloat("u_open", m_fromTear ? 1.0f : (float) std::min(elapsed / 0.9, 1.0));
    m_shader->SetFloat("u_close", (float) std::max(close, 0.0));
    m_shader->SetFloat("u_stability", m_stability);
    m_shader->SetFloat("u_time", (float) fmod(elapsed, 600.0));

    device->BindVertexArray(m_vao);
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);

    Everstead::GetInstance()->GetFrameFramebuffer()->Bind();
    device->SetViewport(0, 0, width, height);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_One, eBlendFactor_OneMinusSrcAlpha);
    device->SetTextureFilter(m_target->GetColorTexture(), eTextureFilter_Linear, eTextureFilter_Linear);
    m_compositeShader->Bind();
    Textures::Bind(m_target->GetColorTexture(), 0);
    m_compositeShader->SetFloat("u_gammaExponent", 1.0f);
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->BindVertexArray({});

    GlStateManager::SetBlendFunc(blendSrc, blendDst);
    if (depthTest)
    {
        GlStateManager::EnableDepthTest();
    }

    if (cull)
    {
        GlStateManager::EnableCull();
    }

    if (!blend)
    {
        GlStateManager::DisableBlend();
    }
}

#endif
