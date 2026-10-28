#pragma once

#include "NimbusSettings.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Framebuffer;
class Shader;
class Texture;

class NimbusClouds
{
public:
    NimbusClouds(int width, int height, const NimbusSettings *settings);
    ~NimbusClouds();

    void Resize(int width, int height);
    void Render(Framebuffer *outputFramebuffer, Texture *cloudTexture, const glm::dmat4 &invViewProjection, const glm::vec3 &cloudColor,
                const glm::vec2 &cloudCell, float cloudBottom, const glm::vec4 &flash, float rain, const glm::vec3 &sunDirection, float maxDistance,
                double time);
    Shader *GetShader() const;

private:
    const NimbusSettings *m_settings;
    Framebuffer *m_framebuffer;
    Shader *m_shader;
    Shader *m_compositeShader;
};
