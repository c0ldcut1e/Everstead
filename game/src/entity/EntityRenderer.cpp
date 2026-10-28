#ifndef EVERSTEAD_DEDICATED_SERVER

#include "EntityRenderer.h"

#include <cmath>

#include "../core/Everstead.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/Tesselator.h"
#include "../rendering/Texture.h"
#include "../rendering/Textures.h"
#include "../scene/Camera.h"
#include "../world/Level.h"
#include "../world/block/Block.h"
#include "Entity.h"

EntityRenderer::EntityRenderer()
{
    m_renderBoundingBox = false;
    m_shadowRadius      = 0.0f;
}

void EntityRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    (void) lightColor;

    RenderFire(entity, partialTicks);

    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

void EntityRenderer::SetRenderBoundingBox(bool value)
{
    m_renderBoundingBox = value;
}

float EntityRenderer::GetShadowRadius() const
{
    return m_shadowRadius;
}

void EntityRenderer::RenderBoundingBox(const Entity *entity, float partialTicks) const
{
    const glm::dvec3 &origin = Everstead::GetInstance()->GetCamera()->GetPosition();
    AABB box                 = entity->GetBoundingBox().Translated(entity->GetRenderPosition(partialTicks) - origin);
    const glm::dvec3 &min    = box.GetMin();
    const glm::dvec3 &max    = box.GetMax();

    GlStateManager::PushMatrix();
    GlStateManager::Translated(origin.x, origin.y, origin.z);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Lines);
    builder->Color(0xFFFFFFFF);

    builder->Vertex(min.x, min.y, min.z);
    builder->Vertex(max.x, min.y, min.z);

    builder->Vertex(max.x, min.y, min.z);
    builder->Vertex(max.x, min.y, max.z);

    builder->Vertex(max.x, min.y, max.z);
    builder->Vertex(min.x, min.y, max.z);

    builder->Vertex(min.x, min.y, max.z);
    builder->Vertex(min.x, min.y, min.z);

    builder->Vertex(min.x, max.y, min.z);
    builder->Vertex(max.x, max.y, min.z);

    builder->Vertex(max.x, max.y, min.z);
    builder->Vertex(max.x, max.y, max.z);

    builder->Vertex(max.x, max.y, max.z);
    builder->Vertex(min.x, max.y, max.z);

    builder->Vertex(min.x, max.y, max.z);
    builder->Vertex(min.x, max.y, min.z);

    builder->Vertex(min.x, min.y, min.z);
    builder->Vertex(min.x, max.y, min.z);

    builder->Vertex(max.x, min.y, min.z);
    builder->Vertex(max.x, max.y, min.z);

    builder->Vertex(max.x, min.y, max.z);
    builder->Vertex(max.x, max.y, max.z);

    builder->Vertex(min.x, min.y, max.z);
    builder->Vertex(min.x, max.y, max.z);

    builder->End();

    GlStateManager::PopMatrix();
}

void EntityRenderer::RenderFire(const Entity *entity, float partialTicks) const
{
    if (!entity->GetSharedFlag(eEntityFlag_OnFire) || !entity->GetLevel())
    {
        return;
    }

    Block *fire              = Block::ByName("fire");
    Block::UVRect layers[2]  = {fire->GetAtlasUVRect(Direction::up), fire->GetSecondaryAtlasUVRect()};
    const Camera *camera     = Everstead::GetInstance()->GetCamera();
    const glm::dvec3 &origin = camera->GetPosition();
    const glm::dvec3 &front  = camera->GetFront();
    const AABB &box          = entity->GetBoundingBox();
    glm::dvec3 center        = entity->GetRenderPosition(partialTicks) - origin;

    // We push camera facing layers out toward the viewer so the body never hides them
    float rightX = (float) -front.z;
    float rightZ = (float) front.x;
    float length = sqrtf(rightX * rightX + rightZ * rightZ);
    if (length < 0.0001f)
    {
        rightX = 1.0f;
        rightZ = 0.0f;
    }
    else
    {
        rightX /= length;
        rightZ /= length;
    }

    float width     = (float) (box.GetMax().x - box.GetMin().x) * 1.4f;
    float height    = (float) (box.GetMax().y - box.GetMin().y) / width;
    float scale     = width;
    float y         = 0.0f;
    float depth     = -0.3f + (float) (int) height * 0.02f;
    int layer       = 0;
    float halfWidth = 0.5f;

    GlStateManager::PushMatrix();
    GlStateManager::Translated(origin.x, origin.y, origin.z);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    // We cut out the empty texels so they never write depth over the layers behind
    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.1f);

    Textures::Bind(fire->GetTexture(Direction::up)->GetId());
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    while (height > 0.0f)
    {
        const Block::UVRect &uv = layers[layer % 2];
        bool flip               = layer / 2 % 2 != 0;
        float u0                = flip ? uv.u1 : uv.u0;
        float u1                = flip ? uv.u0 : uv.u1;
        float v0                = uv.v0;
        float v1                = uv.v1;

        float offsetX = rightZ * depth * scale;
        float offsetZ = -rightX * depth * scale;
        float ax      = (float) center.x - rightX * halfWidth * scale + offsetX;
        float az      = (float) center.z - rightZ * halfWidth * scale + offsetZ;
        float bx      = (float) center.x + rightX * halfWidth * scale + offsetX;
        float bz      = (float) center.z + rightZ * halfWidth * scale + offsetZ;
        float y0      = (float) center.y + y * scale;
        float y1      = y0 + 1.4f * scale;

        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(0xFFFFFFFF);
        builder->VertexUV(ax, y0, az, u1, v1);
        builder->VertexUV(bx, y0, bz, u0, v1);
        builder->VertexUV(bx, y1, bz, u0, v0);
        builder->VertexUV(ax, y0, az, u1, v1);
        builder->VertexUV(bx, y1, bz, u0, v0);
        builder->VertexUV(ax, y1, az, u1, v0);
        builder->End();

        height -= 0.45f;
        y += 0.45f;
        halfWidth *= 0.9f;
        depth += 0.03f;
        layer++;
    }

    Textures::Unbind();
    GlStateManager::SetAlphaCutoff(alphaCutoff);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::PopMatrix();
}

#endif
