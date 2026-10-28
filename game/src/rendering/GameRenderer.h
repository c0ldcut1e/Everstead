#pragma once

#include <cstdint>

#include "../utils/Random.h"
#include "DimensionTransition.h"
#include "Framebuffer.h"
#include "Shader.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera;
class Level;
class LocalPlayer;

class GameRenderer
{
public:
    static constexpr double kHandFov            = 70.0;
    static constexpr double kPanoramaFov        = 90.0;
    static constexpr double kMaxSceneNearPlane  = 1.0;
    static constexpr int s_nearPlaneClearRadius = 3;

    static const float s_blockFlickerJitter;
    static const float s_blockFlickerDamping;
    static const float s_blockFlickerResponse;
    static const float s_blockFlickerLimit;
    static const float s_thirdPersonDistance;
    static const float s_thirdPersonClipPadding;
    static const float s_thirdPersonClipProbeOffset;

    GameRenderer();
    ~GameRenderer();

    void Init();
    void Shutdown();

    void UpdateCamera(Camera *camera, LocalPlayer *player, float partialTicks);

    void RenderGammaPass(Framebuffer *frameFramebuffer);

    DimensionTransition *GetDimensionTransition() const;

    float GetGamma() const;
    void SetGamma(float gamma);

    void TickLightFlicker();

    void UpdateProjection(int width, int height);
    const glm::dmat4 &GetProjection() const;
    glm::dmat4 BuildProjection(float fovScale) const;
    void UpdateSceneNearPlane(Level *level, const glm::dvec3 &cameraPos);
    static bool IsCameraClearOfBlocks(Level *level, const glm::dvec3 &cameraPos);
    glm::dmat4 BuildHandProjection() const;
    glm::dmat4 GetViewMatrix(const Camera *camera) const;
    glm::dmat4 GetViewBobbing(LocalPlayer *player, float partialTicks) const;
    void SetPanoramaCapture(bool capturing);
    bool IsPanoramaCapture() const;

    float GetFov() const;
    void SetFov(float fovDegrees);

    double GetNearPlane() const;
    void SetNearPlane(double nearPlane);

    double GetFarPlane() const;
    void SetFarPlane(double farPlane);

private:
    Shader *m_gammaShader;
    VertexArrayHandle m_gammaFullscreenVao;
    DimensionTransition *m_dimensionTransition;
    float m_gamma;

    glm::dmat4 m_projection;
    int m_projectionWidth;
    int m_projectionHeight;
    float m_fov;
    bool m_panoramaCapture;
    double m_nearPlane;
    double m_sceneNearPlane;
    double m_farPlane;

    Random m_random;

    float m_blockFlicker;
    float m_blockFlickerTarget;
};
