#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Lighting.h"

#include <algorithm>
#include <cmath>

#include "../../rendering/GlStateManager.h"

#include <glm/vec3.hpp>

uint32_t Lighting::s_entityLight = 0xFFFFFFFF;
bool Lighting::s_enabled         = false;
bool Lighting::s_eyeLights       = false;
float Lighting::s_eyeLight0[3]   = {};
float Lighting::s_eyeLight1[3]   = {};

static const float s_ambient = 0.4f;
static const float s_diffuse = 0.6f;

static void RotateNormalized(const glm::dmat4 &matrix, float x, float y, float z, float *out)
{
    float rx     = (float) (matrix[0][0] * x + matrix[1][0] * y + matrix[2][0] * z);
    float ry     = (float) (matrix[0][1] * x + matrix[1][1] * y + matrix[2][1] * z);
    float rz     = (float) (matrix[0][2] * x + matrix[1][2] * y + matrix[2][2] * z);
    float length = sqrtf(rx * rx + ry * ry + rz * rz);
    if (length <= 0.0f)
    {
        length = 1.0f;
    }

    out[0] = rx / length;
    out[1] = ry / length;
    out[2] = rz / length;
}

void Lighting::TurnOn()
{
    s_enabled = true;
}

void Lighting::TurnOff()
{
    s_enabled = false;
}

bool Lighting::IsEnabled()
{
    return s_enabled;
}

void Lighting::SetEntityLight(uint32_t lightColor)
{
    s_entityLight = lightColor;
}

uint32_t Lighting::GetEntityLight()
{
    return s_entityLight;
}

// This is two fixed world space lights over ambient so model shading follows the model's rotation
float Lighting::GetDiffuse(const glm::dmat4 &matrix, float normalX, float normalY, float normalZ)
{
    if (s_eyeLights)
    {
        return GetEyeDiffuse(normalX, normalY, normalZ);
    }

    float x      = matrix[0][0] * normalX + matrix[1][0] * normalY + matrix[2][0] * normalZ;
    float y      = matrix[0][1] * normalX + matrix[1][1] * normalY + matrix[2][1] * normalZ;
    float z      = matrix[0][2] * normalX + matrix[1][2] * normalY + matrix[2][2] * normalZ;
    float length = sqrtf(x * x + y * y + z * z);
    glm::dvec3 normal((double) (x / length), (double) (y / length), (double) (z / length));
    glm::dvec3 light0 = glm::normalize(glm::dvec3(0.2, 1.0, -0.7));
    glm::dvec3 light1 = glm::normalize(glm::dvec3(-0.2, 1.0, 0.7));
    float lit         = (float) (std::max(0.0, glm::dot(normal, light0)) + std::max(0.0, glm::dot(normal, light1)));
    return std::min(1.0f, s_ambient + s_diffuse * lit);
}

// One light sample covers the whole entity, per vertex samples split limbs across blocks with different light
uint32_t Lighting::ShadeColor(uint32_t color, float brightness)
{
    uint32_t a = (color >> 24) & 0xFF;
    uint32_t r = (uint32_t) (((color >> 16) & 0xFF) * brightness);
    uint32_t g = (uint32_t) (((color >> 8) & 0xFF) * brightness);
    uint32_t b = (uint32_t) ((color & 0xFF) * brightness);

    if (!s_enabled)
    {
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    uint32_t lr = (s_entityLight >> 16) & 0xFF;
    uint32_t lg = (s_entityLight >> 8) & 0xFF;
    uint32_t lb = s_entityLight & 0xFF;

    r = (r * lr + 127) / 255;
    g = (g * lg + 127) / 255;
    b = (b * lb + 127) / 255;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

// The world lights are carried into view space, so first person geometry shades by where the camera looks
void Lighting::SetEyeLights(const glm::dmat4 &view)
{
    s_eyeLights = true;
    RotateNormalized(view, 0.2f, 1.0f, -0.7f, s_eyeLight0);
    RotateNormalized(view, -0.2f, 1.0f, 0.7f, s_eyeLight1);
}

void Lighting::ClearEyeLights()
{
    s_eyeLights = false;
}

bool Lighting::HasEyeLights()
{
    return s_eyeLights;
}

// The normal is local to the current model view matrix, which already sits in view space
float Lighting::GetEyeDiffuse(float normalX, float normalY, float normalZ)
{
    float normal[3];
    RotateNormalized(GlStateManager::GetMatrix(), normalX, normalY, normalZ, normal);
    float dot0 = normal[0] * s_eyeLight0[0] + normal[1] * s_eyeLight0[1] + normal[2] * s_eyeLight0[2];
    float dot1 = normal[0] * s_eyeLight1[0] + normal[1] * s_eyeLight1[1] + normal[2] * s_eyeLight1[2];
    float lit  = std::max(0.0f, dot0) + std::max(0.0f, dot1);
    return std::min(1.0f, s_ambient + s_diffuse * lit);
}

#endif
