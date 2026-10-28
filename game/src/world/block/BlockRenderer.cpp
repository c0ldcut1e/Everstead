#ifndef EVERSTEAD_DEDICATED_SERVER

#include "BlockRenderer.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "../../rendering/BlockAtlas.h"
#include "../../rendering/BufferBuilder.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Texture.h"
#include "../../rendering/Textures.h"
#include "../../utils/Direction.h"
#include "../biome/Biome.h"
#include "../blockentity/renderer/BlockEntityRendererRegistry.h"
#include "../lighting/Lighting.h"
#include "BlockRegistry.h"

float BlockRenderer::xOff = 0.0f;
float BlockRenderer::yOff = 0.0f;
float BlockRenderer::zOff = 0.0f;

bool BlockRenderer::IsWaterBlock(Block *block, Block *waterStillBlock, Block *waterFlowBlock)
{
    return block && (block == waterStillBlock || block == waterFlowBlock);
}

bool BlockRenderer::TesselateWaterInWorld(Block *block, Block *neighbor, Block *waterStillBlock, Block *waterFlowBlock)
{
    if (!IsWaterBlock(block, waterStillBlock, waterFlowBlock))
    {
        return false;
    }

    if (IsWaterBlock(neighbor, waterStillBlock, waterFlowBlock))
    {
        return false;
    }

    if (!neighbor)
    {
        return true;
    }

    if (neighbor->GetRenderShape() != Block::eRenderShape_CUBE)
    {
        return true;
    }

    return !neighbor->IsSolid() || neighbor->IsLiquid() || neighbor->IsTransparent();
}

bool BlockRenderer::TesselateInWorld(Block *block, Block *neighbor, Block *waterStillBlock, Block *waterFlowBlock)
{
    if (!block)
    {
        return false;
    }

    if (block->GetRenderShape() != Block::eRenderShape_CUBE)
    {
        return false;
    }

    if (TesselateWaterInWorld(block, neighbor, waterStillBlock, waterFlowBlock))
    {
        return true;
    }

    if (block->IsLiquid())
    {
        return false;
    }

    if (!neighbor)
    {
        return true;
    }

    if (neighbor->GetRenderShape() != Block::eRenderShape_CUBE)
    {
        return true;
    }

    if (neighbor->IsLiquid())
    {
        return true;
    }

    if (neighbor == block && block->CullsSameNeighbor())
    {
        return false;
    }

    if (neighbor->IsSolid() && !neighbor->IsTransparent())
    {
        return false;
    }

    return true;
}

// Held in first person the faces take the view space lights, everywhere else they keep the fixed shade
static float ItemFaceBrightness(Direction *direction, float brightness)
{
    if (!Lighting::HasEyeLights())
    {
        return brightness;
    }

    return Lighting::GetEyeDiffuse((float) direction->dx, (float) direction->dy, (float) direction->dz);
}

static void EmitItemCubeFace(BufferBuilder *builder, Block *block, uint16_t data, Direction *direction, float brightness, uint32_t color, float x0,
                             float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                             float inset)
{
    brightness       = ItemFaceBrightness(direction, brightness);
    Texture *texture = block->GetTexture(direction);
    if (!texture)
    {
        return;
    }

    uint32_t tint = block->HasBiomeTint(direction) ? block->GetItemTint() : 0xFFFFFF;

    uint32_t a = (color >> 24) & 0xFF;
    uint32_t r = (uint32_t) ((float) (((color >> 16) & 0xFF) * ((tint >> 16) & 0xFF)) / 255.0f * brightness);
    uint32_t g = (uint32_t) ((float) (((color >> 8) & 0xFF) * ((tint >> 8) & 0xFF)) / 255.0f * brightness);
    uint32_t b = (uint32_t) ((float) ((color & 0xFF) * (tint & 0xFF)) / 255.0f * brightness);

    Block::UVRect rect = block->GetAtlasUVRect(direction, data);
    float insetU       = (rect.u1 - rect.u0) * inset;
    float insetV       = (rect.v1 - rect.v0) * inset;
    rect.u0 += insetU;
    rect.u1 -= insetU;
    rect.v0 += insetV;
    rect.v1 -= insetV;

    Textures::Bind(texture->GetId());

    builder->Begin(ePrimitiveType_Triangles);
    builder->Color((a << 24) | (r << 16) | (g << 8) | b);
    builder->VertexUV(x0, y0, z0, rect.u0, rect.v1);
    builder->VertexUV(x1, y1, z1, rect.u1, rect.v1);
    builder->VertexUV(x2, y2, z2, rect.u1, rect.v0);
    builder->VertexUV(x0, y0, z0, rect.u0, rect.v1);
    builder->VertexUV(x2, y2, z2, rect.u1, rect.v0);
    builder->VertexUV(x3, y3, z3, rect.u0, rect.v0);
    builder->End();
}

static void EmitItemOverlayFace(BufferBuilder *builder, Block *block, float brightness, uint32_t color, float x0, float y0, float z0, float x1,
                                float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float inset)
{
    Block::UVRect rect = block->GetSideOverlayAtlasUVRect();
    float insetU       = (rect.u1 - rect.u0) * inset;
    float insetV       = (rect.v1 - rect.v0) * inset;
    rect.u0 += insetU;
    rect.u1 -= insetU;
    rect.v0 += insetV;
    rect.v1 -= insetV;

    uint32_t tint = Block::GetItemGrassTint();
    uint32_t a    = (color >> 24) & 0xFF;
    uint32_t r    = (uint32_t) ((float) (((color >> 16) & 0xFF) * ((tint >> 16) & 0xFF)) / 255.0f * brightness);
    uint32_t g    = (uint32_t) ((float) (((color >> 8) & 0xFF) * ((tint >> 8) & 0xFF)) / 255.0f * brightness);
    uint32_t b    = (uint32_t) ((float) ((color & 0xFF) * (tint & 0xFF)) / 255.0f * brightness);

    Textures::Bind(block->GetTexture(Direction::north)->GetId());

    builder->Begin(ePrimitiveType_Triangles);
    builder->Color((a << 24) | (r << 16) | (g << 8) | b);
    builder->VertexUV(x0, y0, z0, rect.u0, rect.v1);
    builder->VertexUV(x1, y1, z1, rect.u1, rect.v1);
    builder->VertexUV(x2, y2, z2, rect.u1, rect.v0);
    builder->VertexUV(x0, y0, z0, rect.u0, rect.v1);
    builder->VertexUV(x2, y2, z2, rect.u1, rect.v0);
    builder->VertexUV(x3, y3, z3, rect.u0, rect.v0);
    builder->End();
}

static void EmitItemBoxFace(BufferBuilder *builder, Block *block, uint16_t data, Direction *direction, float brightness, uint32_t color,
                            const AABB &box, bool doubleSided, float inset)
{
    brightness       = ItemFaceBrightness(direction, brightness);
    Texture *texture = block->GetTexture(direction);
    if (!texture)
    {
        return;
    }

    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();
    double corners[4][3];
    if (direction == Direction::south)
    {
        double quad[4][3] = {{min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}};
        std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
    }
    else if (direction == Direction::north)
    {
        double quad[4][3] = {{max.x, min.y, min.z}, {min.x, min.y, min.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z}};
        std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
    }
    else if (direction == Direction::west)
    {
        double quad[4][3] = {{min.x, min.y, min.z}, {min.x, min.y, max.z}, {min.x, max.y, max.z}, {min.x, max.y, min.z}};
        std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
    }
    else if (direction == Direction::east)
    {
        double quad[4][3] = {{max.x, min.y, max.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {max.x, max.y, max.z}};
        std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
    }
    else if (direction == Direction::up)
    {
        double quad[4][3] = {{min.x, max.y, max.z}, {max.x, max.y, max.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}};
        std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
    }
    else
    {
        double quad[4][3] = {{min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, min.y, max.z}, {min.x, min.y, max.z}};
        std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
    }

    uint32_t tint = block->HasBiomeTint(direction) ? block->GetItemTint() : 0xFFFFFF;
    uint32_t a    = (color >> 24) & 0xFF;
    uint32_t r    = (uint32_t) ((float) (((color >> 16) & 0xFF) * ((tint >> 16) & 0xFF)) / 255.0f * brightness);
    uint32_t g    = (uint32_t) ((float) (((color >> 8) & 0xFF) * ((tint >> 8) & 0xFF)) / 255.0f * brightness);
    uint32_t b    = (uint32_t) ((float) ((color & 0xFF) * (tint & 0xFF)) / 255.0f * brightness);

    double raw[4][2];
    for (int i = 0; i < 4; i++)
    {
        raw[i][1] = 1.0 - corners[i][1];
        if (direction == Direction::south)
        {
            raw[i][0] = corners[i][0];
        }
        else if (direction == Direction::north)
        {
            raw[i][0] = 1.0 - corners[i][0];
        }
        else if (direction == Direction::west)
        {
            raw[i][0] = corners[i][2];
        }
        else if (direction == Direction::east)
        {
            raw[i][0] = 1.0 - corners[i][2];
        }
        else if (direction == Direction::up)
        {
            raw[i][0] = 1.0 - corners[i][0];
            raw[i][1] = 1.0 - corners[i][2];
        }
        else
        {
            raw[i][0] = 1.0 - corners[i][0];
            raw[i][1] = corners[i][2];
        }
    }

    double minU = std::min({raw[0][0], raw[1][0], raw[2][0], raw[3][0]});
    double maxU = std::max({raw[0][0], raw[1][0], raw[2][0], raw[3][0]});
    double minV = std::min({raw[0][1], raw[1][1], raw[2][1], raw[3][1]});
    double maxV = std::max({raw[0][1], raw[1][1], raw[2][1], raw[3][1]});
    float u0    = (float) minU;
    float u1    = (float) maxU;
    float v0    = (float) minV;
    float v1    = (float) maxV;

    bool flipU         = false;
    Block::UVRect rect = block->GetBoxFaceAtlasUVRect(data, box, direction, &flipU);
    if (flipU)
    {
        u0 = 1.0f - u0;
        u1 = 1.0f - u1;
    }

    block->AdjustBoxFaceUV(data, box, direction, &u0, &v0, &u1, &v1);

    float uv[4][2];
    for (int i = 0; i < 4; i++)
    {
        float u  = raw[i][0] > (minU + maxU) * 0.5 ? u1 : u0;
        float v  = raw[i][1] > (minV + maxV) * 0.5 ? v1 : v0;
        uv[i][0] = rect.u0 + (rect.u1 - rect.u0) * std::clamp(u, inset, 1.0f - inset);
        uv[i][1] = rect.v0 + (rect.v1 - rect.v0) * std::clamp(v, inset, 1.0f - inset);
    }

    Textures::Bind(texture->GetId());
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color((a << 24) | (r << 16) | (g << 8) | b);
    int order[12] = {0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2};
    for (int i = 0; i < (doubleSided ? 12 : 6); i++)
    {
        const double *corner = corners[order[i]];
        builder->VertexUV((float) corner[0] - 0.5f, (float) corner[1] - 0.5f, (float) corner[2] - 0.5f, uv[order[i]][0], uv[order[i]][1]);
    }

    builder->End();
}

void BlockRenderer::RenderBlockAsItem(Block *block, uint32_t color)
{
    if (block->IsEntityRendered())
    {
        GlStateManager::EnableCull();
        BlockEntityRendererRegistry::GetInstance()->GetValue(block->GetBlockEntityId())->RenderItem(color);
        Textures::Unbind();
        return;
    }

    uint16_t data         = block->GetItemData();
    TextureAtlas *atlas   = BlockAtlas::GetInstance()->GetBlockAtlas();
    Block::UVRect rects[] = {block->GetAtlasUVRect(Direction::north, data),
                             block->GetAtlasUVRect(Direction::south, data),
                             block->GetAtlasUVRect(Direction::west, data),
                             block->GetAtlasUVRect(Direction::east, data),
                             block->GetAtlasUVRect(Direction::up, data),
                             block->GetAtlasUVRect(Direction::down, data),
                             block->GetCrossAtlasUVRect(data),
                             block->GetSecondaryAtlasUVRect()};
    for (size_t i = 0; i < std::size(rects); i++)
    {
        atlas->MarkVisible({rects[i].u0, rects[i].v0, rects[i].u1, rects[i].v1});
    }

    RenderBlock(block, data, true, color);
}

void BlockRenderer::RenderBlock(Block *block, uint16_t data, bool item, uint32_t color)
{
    // Item icons sample slightly inside each tile, so neighbouring atlas tiles can't bleed into their edges
    float inset = item ? 0.01f : 0.0f;
    if (block->GetRenderShape() == Block::eRenderShape_BOXES)
    {
        float alphaCutoff = GlStateManager::GetAlphaCutoff();
        GlStateManager::SetAlphaCutoff(0.1f);
        GlStateManager::EnableCull();
        GlStateManager::EnableBlend();
        GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

        BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
        std::vector<AABB> boxes;
        if (item)
        {
            block->GetItemBoxes(&boxes);
        }
        else
        {
            block->GetRenderBoxes(data, &boxes);
        }

        Direction *directions[]    = {Direction::south, Direction::north, Direction::west, Direction::east, Direction::up, Direction::down};
        const float brightnesses[] = {0.8f, 0.8f, 0.6f, 0.6f, 1.0f, 0.5f};
        for (size_t i = 0; i < boxes.size(); i++)
        {
            for (int d = 0; d < 6; d++)
            {
                if (block->HasBoxFace(data, i, directions[d]))
                {
                    EmitItemBoxFace(builder, block, data, directions[d], brightnesses[d], color, boxes[i],
                                    block->IsBoxFaceDoubleSided(data, i, directions[d]), inset);
                }
            }
        }

        GlStateManager::SetAlphaCutoff(alphaCutoff);
        Textures::Unbind();
        return;
    }

    const float min = -0.5f;
    const float max = 0.5f;

    GlStateManager::EnableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();

    EmitItemCubeFace(builder, block, data, Direction::south, 0.8f, color, min, min, max, max, min, max, max, max, max, min, max, max, inset);
    EmitItemCubeFace(builder, block, data, Direction::north, 0.8f, color, max, min, min, min, min, min, min, max, min, max, max, min, inset);
    EmitItemCubeFace(builder, block, data, Direction::west, 0.6f, color, min, min, min, min, min, max, min, max, max, min, max, min, inset);
    EmitItemCubeFace(builder, block, data, Direction::east, 0.6f, color, max, min, max, max, min, min, max, max, min, max, max, max, inset);
    EmitItemCubeFace(builder, block, data, Direction::up, 1.0f, color, min, max, max, max, max, max, max, max, min, min, max, min, inset);
    EmitItemCubeFace(builder, block, data, Direction::down, 0.5f, color, min, min, min, max, min, min, max, min, max, min, min, max, inset);

    if (block->HasSideOverlay())
    {
        GlStateManager::EnablePolygonOffsetFill();
        GlStateManager::SetPolygonOffset(-1.0f, -1.0f);
        EmitItemOverlayFace(builder, block, ItemFaceBrightness(Direction::south, 0.8f), color, min, min, max, max, min, max, max, max, max, min, max,
                            max, inset);
        EmitItemOverlayFace(builder, block, ItemFaceBrightness(Direction::north, 0.8f), color, max, min, min, min, min, min, min, max, min, max, max,
                            min, inset);
        EmitItemOverlayFace(builder, block, ItemFaceBrightness(Direction::west, 0.6f), color, min, min, min, min, min, max, min, max, max, min, max,
                            min, inset);
        EmitItemOverlayFace(builder, block, ItemFaceBrightness(Direction::east, 0.6f), color, max, min, max, max, min, min, max, max, min, max, max,
                            max, inset);
        GlStateManager::DisablePolygonOffsetFill();
    }

    Textures::Unbind();
}

void BlockRenderer::RenderBlockAsEntity(Block *block, uint16_t data, uint32_t color)
{
    if (!block)
    {
        return;
    }

    if (block->GetRenderShape() != Block::eRenderShape_CROSS)
    {
        RenderBlock(block, data, false, color);
        return;
    }

    Texture *texture = block->GetTexture(Direction::up);
    if (!texture)
    {
        return;
    }

    Block::UVRect uv = block->GetCrossAtlasUVRect(data);

    const float a       = 0.45f;
    const float x[2][2] = {{-a, a}, {-a, a}};
    const float z[2][2] = {{-a, a}, {a, -a}};

    // Clear pixels must not write depth or they hide water drawn afterwards
    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.1f);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    Textures::Bind(texture->GetId());

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(color);
    for (int i = 0; i < 2; i++)
    {
        builder->VertexUV(x[i][0], -0.5f, z[i][0], uv.u0, uv.v1);
        builder->VertexUV(x[i][1], -0.5f, z[i][1], uv.u1, uv.v1);
        builder->VertexUV(x[i][1], 0.5f, z[i][1], uv.u1, uv.v0);
        builder->VertexUV(x[i][0], -0.5f, z[i][0], uv.u0, uv.v1);
        builder->VertexUV(x[i][1], 0.5f, z[i][1], uv.u1, uv.v0);
        builder->VertexUV(x[i][0], 0.5f, z[i][0], uv.u0, uv.v0);
    }

    builder->End();
    GlStateManager::EnableCull();
    GlStateManager::SetAlphaCutoff(alphaCutoff);
    Textures::Unbind();
}

#endif
