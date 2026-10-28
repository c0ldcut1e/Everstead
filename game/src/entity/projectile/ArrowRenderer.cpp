#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ArrowRenderer.h"

#include <cmath>
#include <numbers>

#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Texture.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../item/ItemEntityRenderer.h"
#include "Arrow.h"

ArrowRenderer::ArrowRenderer()
{
    m_texture = TextureRepository::GetInstance()->GetRaw("assets/texture/entity/arrow/arrow.png");
}

void ArrowRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const Arrow *arrow = (const Arrow *) entity;
    double yaw         = arrow->GetRenderYaw(partialTicks) * std::numbers::pi / 180.0;
    double pitch       = arrow->GetRenderPitch(partialTicks) * std::numbers::pi / 180.0;
    glm::dvec3 forward(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));
    glm::dvec3 side = glm::normalize(glm::cross(forward, glm::dvec3(0.0, 1.0, 0.0)));
    glm::dvec3 up   = glm::cross(side, forward);

    float shake = arrow->GetShake(partialTicks);
    if (shake > 0.0f)
    {
        double angle      = -sinf(shake * 3.0f) * shake * std::numbers::pi / 180.0;
        glm::dvec3 shaken = forward * cos(angle) + up * sin(angle);
        up                = up * cos(angle) - forward * sin(angle);
        forward           = shaken;
    }

    const double pixel = 0.05625;
    glm::dvec3 center  = arrow->GetRenderPosition(partialTicks);
    if (arrow->IsCollecting())
    {
        const Entity *collector = ItemEntityRenderer::FindCollector(arrow->GetCollectorIndex());
        if (collector)
        {
            glm::dvec3 target = collector->GetRenderPosition(partialTicks) + glm::dvec3(0.0, 0.5, 0.0);
            center += (target - center) * (double) arrow->GetCollectProgress(partialTicks);
        }
    }

    glm::dvec3 tail = center + forward * (-12.0 * pixel);
    glm::dvec3 tip  = center + forward * (4.0 * pixel);
    glm::dvec3 finA = (side + up) * (2.0 * pixel / sqrt(2.0));
    glm::dvec3 finB = (up - side) * (2.0 * pixel / sqrt(2.0));
    glm::dvec3 back = center + forward * (-11.0 * pixel);

    GlStateManager::DisableCull();
    GlStateManager::SetAlphaCutoff(0.1f);
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(m_texture->GetId());
    builder->Color(lightColor);
    glm::dvec3 fins[2] = {finA, finB};
    for (int i = 0; i < 2; i++)
    {
        glm::dvec3 a = tail - fins[i];
        glm::dvec3 b = tip - fins[i];
        glm::dvec3 c = tip + fins[i];
        glm::dvec3 d = tail + fins[i];
        builder->VertexUV(a.x, a.y, a.z, 0.0f, 5.0f / 32.0f);
        builder->VertexUV(b.x, b.y, b.z, 0.5f, 5.0f / 32.0f);
        builder->VertexUV(c.x, c.y, c.z, 0.5f, 0.0f);
        builder->VertexUV(a.x, a.y, a.z, 0.0f, 5.0f / 32.0f);
        builder->VertexUV(c.x, c.y, c.z, 0.5f, 0.0f);
        builder->VertexUV(d.x, d.y, d.z, 0.0f, 0.0f);
    }

    glm::dvec3 a = back - finA - finB;
    glm::dvec3 b = back + finA - finB;
    glm::dvec3 c = back + finA + finB;
    glm::dvec3 d = back - finA + finB;
    builder->VertexUV(a.x, a.y, a.z, 0.0f, 10.0f / 32.0f);
    builder->VertexUV(b.x, b.y, b.z, 5.0f / 32.0f, 10.0f / 32.0f);
    builder->VertexUV(c.x, c.y, c.z, 5.0f / 32.0f, 5.0f / 32.0f);
    builder->VertexUV(a.x, a.y, a.z, 0.0f, 10.0f / 32.0f);
    builder->VertexUV(c.x, c.y, c.z, 5.0f / 32.0f, 5.0f / 32.0f);
    builder->VertexUV(d.x, d.y, d.z, 0.0f, 5.0f / 32.0f);
    builder->End();
    GlStateManager::SetAlphaCutoff(-1.0f);
    GlStateManager::EnableCull();

    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
