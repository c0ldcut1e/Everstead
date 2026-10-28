#ifndef EVERSTEAD_DEDICATED_SERVER

#include "SignRenderer.h"

#include <cmath>
#include <numbers>

#include "../../../rendering/GlStateManager.h"
#include "../../../rendering/Tesselator.h"
#include "../../../rendering/Texture.h"
#include "../../../rendering/TextureRepository.h"
#include "../../../rendering/Textures.h"
#include "../../Level.h"
#include "../../block/SignBlock.h"
#include "../../lighting/Lighting.h"
#include "../BlockEntity.h"

#include <glm/gtc/matrix_transform.hpp>

void SignRenderer::Render(const BlockEntity *blockEntity, float partialTicks) const
{
    (void) partialTicks;

    const BlockPos &pos = blockEntity->GetPos();
    Level *level        = blockEntity->GetLevel();
    SignBlock *block    = (SignBlock *) Block::ById(level->GetBlockId(pos));
    RenderModel(pos.x, pos.y, pos.z, block, block->GetYRot(level->GetBlockData(pos)), 0xFFFFFFFF);
}

void SignRenderer::RenderItem(uint32_t color) const
{
    RenderModel(-0.5, -0.5, -0.5, (SignBlock *) Block::ByName("oak_sign"), 0.0f, color);
}

void SignRenderer::RenderModel(double x, double y, double z, SignBlock *block, float yRot, uint32_t color) const
{
    const float modelScale = 2.0f / 3.0f;

    GlStateManager::PushMatrix();
    GlStateManager::Translated(x + 0.5, y + 0.75 * modelScale, z + 0.5);
    GlStateManager::Rotatef(-yRot, 0.0f, 1.0f, 0.0f);
    glm::dmat4 matrix = glm::rotate(glm::dmat4(1.0), -yRot * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    if (block->IsWall())
    {
        GlStateManager::Translatef(0.0f, -0.3125f, -0.4375f);
    }

    GlStateManager::Scalef(modelScale, -modelScale, -modelScale);
    matrix = glm::scale(matrix, glm::dvec3(1.0, -1.0, -1.0));

    Textures::Bind(TextureRepository::GetInstance()->GetRaw(block->GetEntityTexturePath())->GetId());
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    RenderBox(builder, matrix, color, -12.0f, -14.0f, -1.0f, 24, 12, 2, 0, 0);
    if (!block->IsWall())
    {
        RenderBox(builder, matrix, color, -1.0f, -2.0f, -1.0f, 2, 14, 2, 0, 14);
    }

    GlStateManager::PopMatrix();
}

void SignRenderer::RenderBox(BufferBuilder *builder, const glm::dmat4 &matrix, uint32_t color, float boxX, float boxY, float boxZ, int w, int h,
                             int d, int texU, int texV) const
{
    struct Face
    {
        int corners[4];
        int u0;
        int v0;
        int u1;
        int v1;
        float normal[3];
    };

    const Face faces[] = {
            {{5, 1, 2, 6}, d + w, d, d + w + d, d + h, {1.0f, 0.0f, 0.0f}},
            {{0, 4, 7, 3}, 0, d, d, d + h, {-1.0f, 0.0f, 0.0f}},
            {{5, 4, 0, 1}, d, 0, d + w, d, {0.0f, -1.0f, 0.0f}},
            {{2, 3, 7, 6}, d + w, d, d + w + w, 0, {0.0f, 1.0f, 0.0f}},
            {{1, 0, 3, 2}, d, d, d + w, d + h, {0.0f, 0.0f, -1.0f}},
            {{4, 5, 6, 7}, d + w + d, d, d + w + d + w, d + h, {0.0f, 0.0f, 1.0f}},
    };

    float x0                  = boxX / 16.0f;
    float y0                  = boxY / 16.0f;
    float z0                  = boxZ / 16.0f;
    float x1                  = (boxX + w) / 16.0f;
    float y1                  = (boxY + h) / 16.0f;
    float z1                  = (boxZ + d) / 16.0f;
    const float corners[8][3] = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0}, {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};

    builder->Begin(ePrimitiveType_Triangles);
    for (int i = 0; i < 6; i++)
    {
        const Face &face = faces[i];
        float nx         = matrix[0][0] * face.normal[0] + matrix[1][0] * face.normal[1] + matrix[2][0] * face.normal[2];
        float ny         = matrix[0][1] * face.normal[0] + matrix[1][1] * face.normal[1] + matrix[2][1] * face.normal[2];
        float brightness = fabsf(ny) > 0.5f ? (ny > 0.0f ? 1.0f : 0.5f) : (fabsf(nx) > 0.5f ? 0.6f : 0.8f);
        if (Lighting::IsEnabled())
        {
            brightness = Lighting::GetDiffuse(matrix, face.normal[0], face.normal[1], face.normal[2]);
        }

        builder->Color(Lighting::ShadeColor(color, brightness));

        float u0              = (float) (texU + face.u0);
        float v0              = (float) (texV + face.v0);
        float u1              = (float) (texU + face.u1);
        float v1              = (float) (texV + face.v1);
        float us              = u1 > u0 ? 0.1f : -0.1f;
        float vs              = v1 > v0 ? 0.1f : -0.1f;
        const float uvs[4][2] = {{(u1 - us) / 64.0f, (v0 + vs) / 32.0f},
                                 {(u0 + us) / 64.0f, (v0 + vs) / 32.0f},
                                 {(u0 + us) / 64.0f, (v1 - vs) / 32.0f},
                                 {(u1 - us) / 64.0f, (v1 - vs) / 32.0f}};
        const int order[6]    = {0, 1, 2, 0, 2, 3};
        for (int j = 0; j < 6; j++)
        {
            const float *corner = corners[face.corners[order[j]]];
            builder->VertexUV(corner[0], corner[1], corner[2], uvs[order[j]][0], uvs[order[j]][1]);
        }
    }

    builder->End();
}

#endif
