#pragma once

#include <chrono>

#include <glm/vec3.hpp>

#include "RenderTypes.h"

class Framebuffer;
class Shader;

class DimensionTransition
{
public:
    DimensionTransition();
    ~DimensionTransition();

    void Start();
    void Finish();
    void Stop();
    bool IsCovered() const;
    bool IsTraveling() const;
    double GetElapsedSeconds() const;
    void SetStability(float stability);
    void SetTear(const glm::dvec3 &center, float seed);
    void Render(int width, int height);

private:
    void MoveTearOrigin();

    Shader *m_shader;
    Shader *m_compositeShader;
    Framebuffer *m_target;
    VertexArrayHandle m_vao;
    bool m_active;
    bool m_finished;
    float m_stability;
    glm::dvec3 m_startFront;
    bool m_fromTear;
    float m_tearSeed;
    double m_tearTravel;
    glm::dvec3 m_tearOrigin;
    double m_tearAngle;
    glm::dvec3 m_lastCamera;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_finishTime;
};
