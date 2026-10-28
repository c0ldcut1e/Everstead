#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LightningBoltRenderer.h"

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../scene/Camera.h"
#include "LightningBolt.h"

void LightningBoltRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const int segments       = 8;
    const int segmentHeight  = 16;
    const int layers         = 4;
    const int glowLayers     = 2;
    const int branches       = 3;
    const uint32_t color     = 0x4D73737F;
    const uint32_t glowColor = 0x14707890;
    const double cornerX[4]  = {-1.0, 1.0, 1.0, -1.0};
    const double cornerZ[4]  = {-1.0, -1.0, 1.0, 1.0};

    const LightningBolt *bolt = (const LightningBolt *) entity;
    if (!bolt->IsVisible())
    {
        return;
    }

    const glm::dvec3 &cameraPosition = Everstead::GetInstance()->GetCamera()->GetPosition();
    glm::dvec3 origin                = bolt->GetRenderPosition(partialTicks) - cameraPosition;
    double xOffsets[segments];
    double zOffsets[segments];
    double xOffset = 0.0;
    double zOffset = 0.0;
    Random random((uint64_t) bolt->GetSeed());
    for (int h = segments - 1; h >= 0; h--)
    {
        xOffsets[h] = xOffset;
        zOffsets[h] = zOffset;
        xOffset += random.NextInt(0, 10) - 5;
        zOffset += random.NextInt(0, 10) - 5;
    }

    GlStateManager::DisableTexture();
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_One);
    GlStateManager::SetDepthMask(false);
    GlStateManager::PushMatrix();
    GlStateManager::Translated(cameraPosition.x, cameraPosition.y, cameraPosition.z);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    for (int layer = 0; layer < layers + glowLayers; layer++)
    {
        bool glow = layer >= layers;
        builder->Color(glow ? glowColor : color);
        Random layerRandom((uint64_t) bolt->GetSeed());
        for (int pass = 0; pass <= branches; pass++)
        {
            int top    = pass == 0 ? segments - 1 : segments - 1 - pass;
            int bottom = pass == 0 ? 0 : top - 2;
            double xo0 = xOffsets[top] - xOffset;
            double zo0 = zOffsets[top] - zOffset;
            for (int h = top; h >= bottom; h--)
            {
                double xo1 = xo0;
                double zo1 = zo0;
                if (pass == 0)
                {
                    xo0 += layerRandom.NextInt(0, 10) - 5;
                    zo0 += layerRandom.NextInt(0, 10) - 5;
                }
                else
                {
                    xo0 += layerRandom.NextInt(0, 30) - 15;
                    zo0 += layerRandom.NextInt(0, 30) - 15;
                }

                double radius0 = glow ? 1.0 + (layer - layers) * 0.8 : 0.1 + layer * 0.2;
                double radius1 = radius0;
                if (pass == 0)
                {
                    radius0 *= h * 0.1 + 1.0;
                    radius1 *= (h - 1) * 0.1 + 1.0;
                }

                double y0 = origin.y + h * segmentHeight;
                double y1 = origin.y + (h + 1) * segmentHeight;
                double x0 = origin.x + 0.5 + xo0;
                double z0 = origin.z + 0.5 + zo0;
                double x1 = origin.x + 0.5 + xo1;
                double z1 = origin.z + 0.5 + zo1;
                for (int side = 0; side < 4; side++)
                {
                    int next  = (side + 1) % 4;
                    double ax = cornerX[side] * radius0;
                    double az = cornerZ[side] * radius0;
                    double bx = cornerX[next] * radius0;
                    double bz = cornerZ[next] * radius0;
                    double cx = cornerX[side] * radius1;
                    double cz = cornerZ[side] * radius1;
                    double dx = cornerX[next] * radius1;
                    double dz = cornerZ[next] * radius1;
                    builder->Vertex(x0 + ax, y0, z0 + az);
                    builder->Vertex(x1 + cx, y1, z1 + cz);
                    builder->Vertex(x1 + dx, y1, z1 + dz);
                    builder->Vertex(x0 + ax, y0, z0 + az);
                    builder->Vertex(x1 + dx, y1, z1 + dz);
                    builder->Vertex(x0 + bx, y0, z0 + bz);
                }
            }
        }
    }

    builder->End();

    GlStateManager::PopMatrix();
    GlStateManager::SetDepthMask(true);
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableBlend();
    GlStateManager::EnableCull();
    GlStateManager::EnableTexture();

    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
