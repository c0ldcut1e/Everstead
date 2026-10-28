#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ExperienceOrbRenderer.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../../scene/Camera.h"
#include "ExperienceOrb.h"

ExperienceOrbRenderer::ExperienceOrbRenderer()
{
    m_texture = TextureRepository::GetInstance()->GetRaw("assets/texture/entity/experience_orb.png");
}

void ExperienceOrbRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const ExperienceOrb *orb = (const ExperienceOrb *) entity;
    int icon                 = orb->GetIcon();
    float u0                 = (icon % 4) / 4.0f;
    float v0                 = (icon / 4) / 4.0f;
    float u1                 = u0 + 0.25f;
    float v1                 = v0 + 0.25f;
    float phase              = (orb->GetTotalTickCount() + partialTicks) / 2.0f;
    float red                = (sinf(phase) + 1.0f) * 0.5f;
    float blue               = (sinf(phase + 4.1888f) + 1.0f) * 0.1f;

    const Camera *camera   = Everstead::GetInstance()->GetCamera();
    glm::dvec3 front       = glm::normalize(camera->GetFront());
    glm::dvec3 worldUp     = fabs(front.y) > 0.99 ? glm::dvec3(0.0, 0.0, 1.0) : glm::dvec3(0.0, 1.0, 0.0);
    glm::dvec3 right       = glm::normalize(glm::cross(worldUp, front)) * 0.3;
    glm::dvec3 up          = glm::normalize(glm::cross(front, right)) * 0.3;
    glm::dvec3 center      = orb->GetRenderPosition(partialTicks) + up * 0.25;
    glm::dvec3 bottomLeft  = center - right * 0.5 - up * 0.5;
    glm::dvec3 bottomRight = center + right * 0.5 - up * 0.5;
    glm::dvec3 topRight    = center + right * 0.5 + up * 0.5;
    glm::dvec3 topLeft     = center - right * 0.5 + up * 0.5;

    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableCull();
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(m_texture->GetId());
    builder->Color(red, 1.0f, blue, 0.5f);
    builder->VertexUV(bottomLeft.x, bottomLeft.y, bottomLeft.z, u1, v1);
    builder->VertexUV(bottomRight.x, bottomRight.y, bottomRight.z, u0, v1);
    builder->VertexUV(topRight.x, topRight.y, topRight.z, u0, v0);
    builder->VertexUV(bottomLeft.x, bottomLeft.y, bottomLeft.z, u1, v1);
    builder->VertexUV(topRight.x, topRight.y, topRight.z, u0, v0);
    builder->VertexUV(topLeft.x, topLeft.y, topLeft.z, u1, v0);
    builder->End();
    GlStateManager::EnableCull();
    GlStateManager::DisableBlend();

    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
