#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChestRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../../rendering/GlStateManager.h"
#include "../../../rendering/Tesselator.h"
#include "../../../rendering/Texture.h"
#include "../../../rendering/TextureRepository.h"
#include "../../../rendering/Textures.h"
#include "../../Level.h"
#include "../../block/ChestBlock.h"
#include "../../lighting/Lighting.h"
#include "../ChestBlockEntity.h"

#include <glm/gtc/matrix_transform.hpp>

ChestRenderer::ChestRenderer()
{
    m_texture      = TextureRepository::GetInstance()->GetRaw("assets/texture/entity/chest/normal.png");
    m_largeTexture = TextureRepository::GetInstance()->GetRaw("assets/texture/entity/chest/normal_double.png");
}

void ChestRenderer::Render(const BlockEntity *blockEntity, float partialTicks) const
{
    const ChestBlockEntity *chest = (const ChestBlockEntity *) blockEntity;
    const BlockPos &pos           = chest->GetPos();
    ChestBlock *chestBlock        = (ChestBlock *) Block::ByName("chest");
    uint16_t data                 = chest->GetLevel()->GetBlockData(pos);
    // We look at the real neighbors because a half whose partner state arrived late would draw a single chest inside the double one
    Direction *partner = chestBlock->FindPartnerDirection(chest->GetLevel(), pos);
    if (partner == Direction::north || partner == Direction::west)
    {
        return;
    }

    float openness = chest->GetOpenness(partialTicks);
    if (partner)
    {
        ChestBlockEntity *other = (ChestBlockEntity *) chest->GetLevel()->GetBlockEntity(BlockPos(pos.x + partner->dx, pos.y, pos.z + partner->dz));
        if (other)
        {
            openness = std::max(openness, other->GetOpenness(partialTicks));
        }
    }

    RenderModel(pos.x, pos.y, pos.z, chestBlock->GetFacing(data), partner != nullptr, openness, 0xFFFFFFFF);
}

void ChestRenderer::RenderItem(uint32_t color) const
{
    RenderModel(-0.5, -0.5, -0.5, 1, false, 0.0f, color);
}

void ChestRenderer::RenderModel(double x, double y, double z, int facing, bool large, float openness, uint32_t color) const
{
    const float rotations[] = {180.0f, 0.0f, 90.0f, -90.0f};

    GlStateManager::PushMatrix();
    GlStateManager::Translated(x, y + 1.0, z + 1.0);
    GlStateManager::Scalef(1.0f, -1.0f, -1.0f);
    GlStateManager::Translatef(0.5f, 0.5f, 0.5f);
    glm::dmat4 matrix = glm::scale(glm::dmat4(1.0), glm::dvec3(1.0, -1.0, -1.0));
    if (large && facing == 0)
    {
        GlStateManager::Translatef(1.0f, 0.0f, 0.0f);
    }
    else if (large && facing == 3)
    {
        GlStateManager::Translatef(0.0f, 0.0f, -1.0f);
    }

    GlStateManager::Rotatef(rotations[facing], 0.0f, 1.0f, 0.0f);
    GlStateManager::Translatef(-0.5f, -0.5f, -0.5f);
    matrix = glm::rotate(matrix, rotations[facing] * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));

    float open     = 1.0f - openness;
    float lid      = -(1.0f - open * open * open) * (float) std::numbers::pi / 2.0f;
    int width      = large ? 30 : 14;
    float texWidth = large ? 128.0f : 64.0f;

    Textures::Bind((large ? m_largeTexture : m_texture)->GetId());
    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    RenderBox(builder, matrix, color, 0.0f, 1.0f, 6.0f, 1.0f, 0.0f, 0.0f, 0.0f, width, 10, 14, 0, 19, texWidth);
    GlStateManager::EnablePolygonOffsetFill();
    GlStateManager::SetPolygonOffset(-0.3f, -0.3f);
    RenderBox(builder, matrix, color, lid, 1.0f, 7.0f, 15.0f, 0.0f, -5.0f, -14.0f, width, 5, 14, 0, 0, texWidth);
    GlStateManager::DisablePolygonOffsetFill();
    RenderBox(builder, matrix, color, lid, large ? 16.0f : 8.0f, 7.0f, 15.0f, -1.0f, -2.0f, -15.0f, 2, 4, 1, 0, 0, texWidth);
    GlStateManager::PopMatrix();
}

void ChestRenderer::RenderBox(BufferBuilder *builder, const glm::dmat4 &matrix, uint32_t color, float xRot, float pivotX, float pivotY, float pivotZ,
                              float boxX, float boxY, float boxZ, int w, int h, int d, int texU, int texV, float texWidth) const
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

    GlStateManager::PushMatrix();
    GlStateManager::Translatef(pivotX / 16.0f, pivotY / 16.0f, pivotZ / 16.0f);
    GlStateManager::Rotatef(xRot * 180.0f / (float) std::numbers::pi, 1.0f, 0.0f, 0.0f);
    glm::dmat4 boxMatrix = glm::rotate(matrix, (double) xRot, glm::dvec3(1.0, 0.0, 0.0));

    builder->Begin(ePrimitiveType_Triangles);
    for (int i = 0; i < 6; i++)
    {
        const Face &face = faces[i];
        float nx         = boxMatrix[0][0] * face.normal[0] + boxMatrix[1][0] * face.normal[1] + boxMatrix[2][0] * face.normal[2];
        float ny         = boxMatrix[0][1] * face.normal[0] + boxMatrix[1][1] * face.normal[1] + boxMatrix[2][1] * face.normal[2];
        float brightness = fabsf(ny) > 0.5f ? (ny > 0.0f ? 1.0f : 0.5f) : (fabsf(nx) > 0.5f ? 0.6f : 0.8f);
        if (Lighting::IsEnabled())
        {
            brightness = Lighting::GetDiffuse(boxMatrix, face.normal[0], face.normal[1], face.normal[2]);
        }

        builder->Color(Lighting::ShadeColor(color, brightness));

        float u0              = (float) (texU + face.u0);
        float v0              = (float) (texV + face.v0);
        float u1              = (float) (texU + face.u1);
        float v1              = (float) (texV + face.v1);
        float us              = u1 > u0 ? 0.1f : -0.1f;
        float vs              = v1 > v0 ? 0.1f : -0.1f;
        const float uvs[4][2] = {{(u1 - us) / texWidth, (v0 + vs) / 64.0f},
                                 {(u0 + us) / texWidth, (v0 + vs) / 64.0f},
                                 {(u0 + us) / texWidth, (v1 - vs) / 64.0f},
                                 {(u1 - us) / texWidth, (v1 - vs) / 64.0f}};
        const int order[6]    = {0, 1, 2, 0, 2, 3};
        for (int j = 0; j < 6; j++)
        {
            const float *corner = corners[face.corners[order[j]]];
            builder->VertexUV(corner[0], corner[1], corner[2], uvs[order[j]][0], uvs[order[j]][1]);
        }
    }

    builder->End();
    GlStateManager::PopMatrix();
}

#endif
