#ifndef EVERSTEAD_DEDICATED_SERVER

#include "GameRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../core/Everstead.h"
#include "../entity/item/Boat.h"
#include "../entity/player/LocalPlayer.h"
#include "../scene/Camera.h"
#include "../utils/Direction.h"
#include "../utils/HitResult.h"
#include "../world/Level.h"
#include "../world/block/BedBlock.h"
#include "../world/block/Block.h"
#include "../world/lighting/LightSource.h"
#include "Framebuffer.h"
#include "GlStateManager.h"
#include "RenderDevice.h"
#include "Shader.h"
#include "Textures.h"

#include <glm/common.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/trigonometric.hpp>

const float GameRenderer::s_blockFlickerJitter         = 0.6f;
const float GameRenderer::s_blockFlickerDamping        = 0.8f;
const float GameRenderer::s_blockFlickerResponse       = 0.6f;
const float GameRenderer::s_blockFlickerLimit          = 1.0f;
const float GameRenderer::s_thirdPersonDistance        = 4.0f;
const float GameRenderer::s_thirdPersonClipPadding     = 0.1f;
const float GameRenderer::s_thirdPersonClipProbeOffset = 0.1f;

GameRenderer::GameRenderer()
{
    m_gammaShader         = nullptr;
    m_gammaFullscreenVao  = {};
    m_dimensionTransition = nullptr;
    m_gamma               = 50.0f;
    m_projectionWidth     = 0;
    m_projectionHeight    = 0;
    m_fov                 = 70.0f;
    m_panoramaCapture     = false;
    m_nearPlane           = 0.05;
    m_sceneNearPlane      = 0.05;
    m_farPlane            = 3000.0;
    m_blockFlicker        = 0.0f;
    m_blockFlickerTarget  = 0.0f;
}

GameRenderer::~GameRenderer()
{
    Shutdown();
}

void GameRenderer::Init()
{
    m_gammaShader         = new Shader("assets/shader/gamma.vert", "assets/shader/gamma.frag");
    m_gammaFullscreenVao  = RenderDevice::GetInstance()->CreateVertexArray();
    m_dimensionTransition = new DimensionTransition();
}

void GameRenderer::Shutdown()
{
    delete m_dimensionTransition;
    m_dimensionTransition = nullptr;

    if (m_gammaFullscreenVao.IsValid())
    {
        RenderDevice::GetInstance()->DeleteVertexArray(m_gammaFullscreenVao);
        m_gammaFullscreenVao = {};
    }

    if (m_gammaShader)
    {
        delete m_gammaShader;
        m_gammaShader = nullptr;
    }
}

void GameRenderer::UpdateCamera(Camera *camera, LocalPlayer *player, float partialTicks)
{
    glm::dvec3 eyePos = player->GetEyePosition(partialTicks);
    float yaw         = player->GetYaw();
    float pitch       = player->GetPitch();
    // A boat turns the rider once per tick, so we take back the part of that turn the vehicle has not rendered yet
    if (Boat *boat = dynamic_cast<Boat *>(player->GetVehicle()))
    {
        yaw += boat->GetRenderYaw(partialTicks) - boat->GetYaw();
    }

    BedBlock *bed = player->IsSleeping() ? dynamic_cast<BedBlock *>(Block::ById(player->GetLevel()->GetBlockId(player->GetBedPos()))) : nullptr;
    if (bed)
    {
        Direction *head = bed->GetHeadDirection(player->GetLevel()->GetBlockData(player->GetBedPos()));
        eyePos          = player->GetRenderPosition(partialTicks) + glm::dvec3(head->dx * 0.6, 0.3, head->dz * 0.6);
        yaw             = glm::degrees(atan2f((float) -head->dz, (float) -head->dx));
        pitch           = 0.0f;
    }

    float yawRad   = yaw * (float) (std::numbers::pi / 180.0);
    float pitchRad = pitch * (float) (std::numbers::pi / 180.0);

    glm::dvec3 front(cos(yawRad) * cos(pitchRad), sin(pitchRad), sin(yawRad) * cos(pitchRad));
    front            = glm::normalize(front);
    glm::dvec3 right = glm::normalize(glm::cross(front, glm::dvec3(0.0, 1.0, 0.0)));
    glm::dvec3 up    = glm::normalize(glm::cross(right, front));

    if (player->GetThirdPersonView() == eThirdPersonView_FirstPerson)
    {
        camera->SetPosition(eyePos);
        camera->SetDirection(front, up);
        UpdateSceneNearPlane(player->GetLevel(), eyePos);
        return;
    }

    glm::dvec3 cameraDirection = front * -1.0;
    if (player->GetThirdPersonView() == eThirdPersonView_Front)
    {
        cameraDirection = front;
    }

    double cameraDistance = s_thirdPersonDistance;
    Level *level          = player->GetLevel();
    for (int i = 0; i < 8; i++)
    {
        float xo             = (float) ((i & 1) * 2 - 1) * s_thirdPersonClipProbeOffset;
        float yo             = (float) (((i >> 1) & 1) * 2 - 1) * s_thirdPersonClipProbeOffset;
        float zo             = (float) (((i >> 2) & 1) * 2 - 1) * s_thirdPersonClipProbeOffset;
        glm::dvec3 clipStart = eyePos + glm::dvec3(xo, yo, zo);

        HitResult *hit = level->Clip(clipStart, cameraDirection, (float) cameraDistance, false, false, true);
        if (hit->IsBlock())
        {
            cameraDistance = std::max(0.0, (double) hit->GetDistance() - (double) s_thirdPersonClipPadding);
        }

        delete hit;
    }

    glm::dvec3 cameraPos   = eyePos + cameraDirection * cameraDistance;
    glm::dvec3 cameraFront = front;
    if (player->GetThirdPersonView() == eThirdPersonView_Front)
    {
        cameraFront = front * -1.0;
    }

    camera->SetPosition(cameraPos);
    camera->SetDirection(cameraFront, up);
    UpdateSceneNearPlane(level, cameraPos);
}

// The near plane corners reach about 2.5 times the near distance out, so every block in that range must be air
bool GameRenderer::IsCameraClearOfBlocks(Level *level, const glm::dvec3 &cameraPos)
{
    int radius = s_nearPlaneClearRadius;
    int blockX = (int) floor(cameraPos.x);
    int blockY = (int) floor(cameraPos.y);
    int blockZ = (int) floor(cameraPos.z);
    for (int y = blockY - radius; y <= blockY + radius; y++)
    {
        for (int z = blockZ - radius; z <= blockZ + radius; z++)
        {
            for (int x = blockX - radius; x <= blockX + radius; x++)
            {
                if (level->GetBlockId(BlockPos(x, y, z)) != 0)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

// Far above the terrain nothing is close, so the near plane moves out to keep depth precision for distant surfaces
void GameRenderer::UpdateSceneNearPlane(Level *level, const glm::dvec3 &cameraPos)
{
    double nearPlane = m_nearPlane;
    if (level && IsCameraClearOfBlocks(level, cameraPos))
    {
        int blockX       = (int) floor(cameraPos.x);
        int blockZ       = (int) floor(cameraPos.z);
        double clearance = cameraPos.y - (double) (level->GetSurfaceHeight(blockX, blockZ) + 1);
        nearPlane        = std::clamp(clearance * 0.25, m_nearPlane, kMaxSceneNearPlane);
        // The border wall is not terrain, so the near plane has to stay well in front of it or it gets clipped away
        double borderDistance = fabs(level->GetWorldBorder().GetDistanceToBorder(cameraPos.x, cameraPos.z));
        nearPlane             = std::clamp(borderDistance * 0.25, m_nearPlane, nearPlane);

        const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
        LocalPlayer *localPlayer                             = Everstead::GetInstance()->GetLocalPlayer();
        for (size_t i = 0; i < entities.size(); i++)
        {
            if (entities[i].get() == localPlayer)
            {
                continue;
            }

            const AABB &box = entities[i]->GetAABB();
            glm::dvec3 closest(std::clamp(cameraPos.x, box.GetMin().x, box.GetMax().x), std::clamp(cameraPos.y, box.GetMin().y, box.GetMax().y),
                               std::clamp(cameraPos.z, box.GetMin().z, box.GetMax().z));
            nearPlane = std::clamp(sqrt(glm::distance2(closest, cameraPos)) * 0.25, m_nearPlane, nearPlane);
        }
    }

    if (fabs(nearPlane - m_sceneNearPlane) < 0.01)
    {
        return;
    }

    m_sceneNearPlane = nearPlane;
    UpdateProjection(m_projectionWidth, m_projectionHeight);
}

void GameRenderer::RenderGammaPass(Framebuffer *frameFramebuffer)
{
    if (!frameFramebuffer || !m_gammaShader || !m_gammaFullscreenVao.IsValid())
    {
        return;
    }

    float gammaT        = std::clamp(m_gamma / 100.0f, 0.0f, 1.0f);
    float gammaExponent = glm::mix(1.35f, 0.60f, gammaT);

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::DisableBlend();

    m_gammaShader->Bind();

    Textures::Bind(frameFramebuffer->GetColorTexture(), 0);
    m_gammaShader->SetFloat("u_gammaExponent", gammaExponent);

    RenderDevice *device = RenderDevice::GetInstance();
    device->BindVertexArray(m_gammaFullscreenVao);
    device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
    device->BindVertexArray({});

    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
    GlStateManager::EnableCull();
}

DimensionTransition *GameRenderer::GetDimensionTransition() const
{
    return m_dimensionTransition;
}

float GameRenderer::GetGamma() const
{
    return m_gamma;
}

void GameRenderer::SetGamma(float gamma)
{
    m_gamma = std::clamp(gamma, 0.0f, 100.0f);
}

void GameRenderer::TickLightFlicker()
{
    float r1 = m_random.NextFloat();
    float r2 = m_random.NextFloat();
    float r3 = m_random.NextFloat();
    float r4 = m_random.NextFloat();

    float jitter         = (r1 - r2) * r3 * r4 * s_blockFlickerJitter;
    m_blockFlickerTarget = std::clamp((m_blockFlickerTarget + jitter) * s_blockFlickerDamping, -s_blockFlickerLimit, s_blockFlickerLimit);
    m_blockFlicker += (m_blockFlickerTarget - m_blockFlicker) * s_blockFlickerResponse;

    LightSource::s_blockFlicker = m_blockFlicker;
}

void GameRenderer::UpdateProjection(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    m_projectionWidth  = width;
    m_projectionHeight = height;

    m_projection = glm::perspective(m_fov * (std::numbers::pi / 180.0), (double) width / (double) height, m_sceneNearPlane, m_farPlane);
}

glm::dmat4 GameRenderer::BuildProjection(float fovScale) const
{
    if (m_panoramaCapture)
    {
        return glm::perspective(kPanoramaFov * (std::numbers::pi / 180.0), 1.0, m_sceneNearPlane, m_farPlane);
    }

    if (m_projectionWidth <= 0 || m_projectionHeight <= 0)
    {
        return m_projection;
    }

    return glm::perspective(m_fov * fovScale * (std::numbers::pi / 180.0), (double) m_projectionWidth / (double) m_projectionHeight, m_sceneNearPlane,
                            m_farPlane);
}

glm::dmat4 GameRenderer::BuildHandProjection() const
{
    return glm::perspective(kHandFov * (std::numbers::pi / 180.0), (double) m_projectionWidth / (double) std::max(m_projectionHeight, 1), m_nearPlane,
                            m_farPlane);
}

const glm::dmat4 &GameRenderer::GetProjection() const
{
    return m_projection;
}

glm::dmat4 GameRenderer::GetViewMatrix(const Camera *camera) const
{
    if (!camera)
    {
        return glm::dmat4(1.0);
    }

    return camera->GetViewMatrix();
}

glm::dmat4 GameRenderer::GetViewBobbing(LocalPlayer *player, float partialTicks) const
{
    if (m_panoramaCapture)
    {
        return glm::dmat4(1.0);
    }

    float pi             = (float) std::numbers::pi;
    float toRadians      = pi / 180.0f;
    Everstead *everstead = Everstead::GetInstance();
    glm::dmat4 transform = everstead->GetLevelRenderer()->GetWorldEffects().GetShake(everstead->GetCamera()->GetPosition());
    transform            = glm::rotate(transform, (double) (-player->GetHurtAnim(partialTicks) * 14.0f * toRadians), glm::dvec3(0.0, 0.0, 1.0));
    if (player->GetHealth() <= 0)
    {
        float deathTime = player->GetRenderPlayer()->GetDeathTime() + partialTicks;
        transform =
                glm::rotate(glm::dmat4(1.0), (double) ((40.0f - 8000.0f / (deathTime + 200.0f)) * toRadians), glm::dvec3(0.0, 0.0, 1.0)) * transform;
    }

    if (!everstead->options.viewBobbing)
    {
        return transform;
    }

    float wda   = player->GetWalkDist() - player->GetWalkDistOld();
    float phase = -(player->GetWalkDist() + wda * partialTicks);
    float bob   = player->GetWalkBobOld() + (player->GetWalkBob() - player->GetWalkBobOld()) * partialTicks;
    float tilt  = player->GetFallTiltOld() + (player->GetFallTilt() - player->GetFallTiltOld()) * partialTicks;
    transform   = glm::translate(transform, glm::dvec3(sinf(phase * pi) * bob * 0.5f, -fabsf(cosf(phase * pi) * bob), 0.0));
    transform   = glm::rotate(transform, (double) (sinf(phase * pi) * bob * 3.0f * toRadians), glm::dvec3(0.0, 0.0, 1.0));
    transform   = glm::rotate(transform, (double) (fabsf(cosf(phase * pi - 0.2f) * bob) * 5.0f * toRadians), glm::dvec3(1.0, 0.0, 0.0));
    transform   = glm::rotate(transform, (double) (tilt * toRadians), glm::dvec3(1.0, 0.0, 0.0));
    return transform;
}

void GameRenderer::SetPanoramaCapture(bool capturing)
{
    m_panoramaCapture = capturing;
}

bool GameRenderer::IsPanoramaCapture() const
{
    return m_panoramaCapture;
}

float GameRenderer::GetFov() const
{
    return m_fov;
}

void GameRenderer::SetFov(float fovDegrees)
{
    m_fov = std::clamp(fovDegrees, 30.0f, 110.0f);
    UpdateProjection(m_projectionWidth, m_projectionHeight);
}

double GameRenderer::GetNearPlane() const
{
    return m_nearPlane;
}

void GameRenderer::SetNearPlane(double nearPlane)
{
    m_nearPlane = nearPlane;
    UpdateProjection(m_projectionWidth, m_projectionHeight);
}

double GameRenderer::GetFarPlane() const
{
    return m_farPlane;
}

void GameRenderer::SetFarPlane(double farPlane)
{
    m_farPlane = farPlane;
    UpdateProjection(m_projectionWidth, m_projectionHeight);
}

#endif
