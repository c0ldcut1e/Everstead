#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherFaceEmitter.h"
#include "ChunkMesherBinaryBuilder.h"
#include "ChunkMesherCache.h"
#include "ChunkMesherLighting.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "../../utils/math/Mth.h"
#include "../LevelSource.h"
#include "../lighting/LightEngine.h"

uint8_t ChunkMesherFaceEmitter::GetTintMode(uint32_t tintPacked)
{
    return (uint8_t) ((tintPacked >> 24) & 0xFF);
}

uint32_t ChunkMesherFaceEmitter::PackTintMode(uint32_t tintRgb, uint8_t mode)
{
    return (tintRgb & 0x00FFFFFF) | ((uint32_t) mode << 24);
}

float ChunkMesherFaceEmitter::GetDirectionalShade(Direction *direction)
{
    if (direction == Direction::north || direction == Direction::south)
    {
        return 0.8f;
    }

    if (direction == Direction::east || direction == Direction::west)
    {
        return 0.6f;
    }

    if (direction == Direction::down)
    {
        return 0.5f;
    }

    return 1.0f;
}

float ChunkMesherFaceEmitter::GetOldDirectionalShade(Direction *direction)
{
    if (direction == Direction::north || direction == Direction::south)
    {
        return 0.8f;
    }

    if (direction == Direction::east || direction == Direction::west)
    {
        return 0.6f;
    }

    if (direction == Direction::down)
    {
        return 0.5f;
    }

    return 1.0f;
}

uint16_t ChunkMesherFaceEmitter::PackLight(uint8_t r, uint8_t g, uint8_t b)
{
    if (r > 15)
    {
        r = 15;
    }

    if (g > 15)
    {
        g = 15;
    }

    if (b > 15)
    {
        b = 15;
    }

    return (uint16_t) ((r << 8) | (g << 4) | b);
}

void ChunkMesherFaceEmitter::UnpackLight(uint16_t k, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (uint8_t) ((k >> 8) & 15);
    *g = (uint8_t) ((k >> 4) & 15);
    *b = (uint8_t) (k & 15);
}

uint16_t ChunkMesherFaceEmitter::PackRawLight(uint8_t br, uint8_t bg, uint8_t bb, uint8_t sky)
{
    return (uint16_t) ((br & 15) | ((bg & 15) << 4) | ((bb & 15) << 8) | ((sky & 15) << 12));
}

uint32_t ChunkMesherFaceEmitter::PackSmoothLightSums(uint8_t br, uint8_t bg, uint8_t bb, uint8_t sky)
{
    return (uint32_t) br | ((uint32_t) bg << 6) | ((uint32_t) bb << 12) | ((uint32_t) sky << 18);
}

uint8_t ChunkMesherFaceEmitter::UnpackSmoothLightSum(uint32_t packed, int shift)
{
    return (uint8_t) ((packed >> shift) & 63);
}

void ChunkMesherFaceEmitter::Push(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                  std::vector<float> &shades, std::vector<uint32_t> &tints, float x, float y, float z, float u, float v, float r,
                                  float g, float b, uint32_t vertexLight, float shadeMul, uint32_t tint, const Block::UVRect &atlasRect)
{
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
    vertices.push_back(u);
    vertices.push_back(v);
    vertices.push_back(r);
    vertices.push_back(g);
    vertices.push_back(b);

    atlasRects.push_back(atlasRect.u0);
    atlasRects.push_back(atlasRect.v0);
    atlasRects.push_back(atlasRect.u1);
    atlasRects.push_back(atlasRect.v1);

    rawLights.push_back(vertexLight);
    shades.push_back(shadeMul);
    tints.push_back(tint);
}

uint32_t ChunkMesherFaceEmitter::ToVertexLight(uint16_t rawLight)
{
    uint32_t vertexLight = 0;
    for (int channel = 0; channel < 4; channel++)
    {
        vertexLight |= (uint32_t) (((rawLight >> (channel * 4)) & 15) * ChunkMesh::kVertexLightScale) << (channel * 8);
    }

    return vertexLight;
}

uint16_t ChunkMesherFaceEmitter::FromVertexLight(uint32_t vertexLight)
{
    uint16_t rawLight = 0;
    for (int channel = 0; channel < 4; channel++)
    {
        uint32_t value = (vertexLight >> (channel * 8)) & 0xFF;
        value          = (value + ChunkMesh::kVertexLightScale / 2) / ChunkMesh::kVertexLightScale;
        rawLight |= (uint16_t) (std::min<uint32_t>(value, 15) << (channel * 4));
    }

    return rawLight;
}

bool ChunkMesherFaceEmitter::SameUVRect(const Block::UVRect &a, const Block::UVRect &b)
{
    return a.u0 == b.u0 && a.v0 == b.v0 && a.u1 == b.u1 && a.v1 == b.v1;
}

void ChunkMesherFaceEmitter::Shade(uint8_t lr, uint8_t lg, uint8_t lb, float shadeMul, float *r, float *g, float *b)
{
    float minLight = 0.03f;
    float br       = (float) lr / 15.0f;
    float bg       = (float) lg / 15.0f;
    float bb       = (float) lb / 15.0f;
    if (br < minLight)
    {
        br = minLight;
    }

    if (bg < minLight)
    {
        bg = minLight;
    }

    if (bb < minLight)
    {
        bb = minLight;
    }

    *r = br * shadeMul;
    *g = bg * shadeMul;
    *b = bb * shadeMul;
}

bool ChunkMesherFaceEmitter::IsWaterAtlasRect(const std::vector<Block::UVRect> &waterAtlasRects, const Block::UVRect &rect)
{
    for (auto waterIt = waterAtlasRects.begin(); waterIt != waterAtlasRects.end(); ++waterIt)
    {
        const Block::UVRect &waterRect = *waterIt;
        if (rect.u0 == waterRect.u0 && rect.v0 == waterRect.v0 && rect.u1 == waterRect.u1 && rect.v1 == waterRect.v1)
        {
            return true;
        }
    }

    return false;
}

void ChunkMesherFaceEmitter::EmitFace(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect,
                                      uint16_t rawLight, uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2)
{
    if (!texture)
    {
        return;
    }

    float uMax = 1.0f;
    float vMax = 1.0f;
    if (direction == Direction::north || direction == Direction::south)
    {
        uMax = fabsf(x2 - x1);
        vMax = fabsf(y2 - y1);
    }
    else if (direction == Direction::east || direction == Direction::west)
    {
        uMax = fabsf(z2 - z1);
        vMax = fabsf(y2 - y1);
    }
    else
    {
        uMax = fabsf(x2 - x1);
        vMax = fabsf(z2 - z1);
    }

    bool useSmooth = context->smoothLighting && !IsWaterAtlasRect(*context->waterAtlasRects, atlasRect);
    float shadeMul = useSmooth ? GetDirectionalShade(direction) : GetOldDirectionalShade(direction);

    MeshBucket &bucket = context->bucketLookup->Get(texture);
    if (!useSmooth)
    {
        float r;
        float g;
        float b;
        uint32_t tintForLighting = GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
        ChunkMesherLighting::ColorFromRawLight(context->level, rawLight, tintForLighting, shadeMul, &r, &g, &b);

        if (direction == Direction::north)
        {
            AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, x1, y2, z1, x2, y2, z1, x2, y1, z1,
                    uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::south)
        {
            AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y1, z2, x2, y2, z2, x1, y2, z2, x1, y1, z2,
                    uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::up)
        {
            AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y2, z1, x1, y2, z2, x2, y2, z2, x2, y2, z1,
                    uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::down)
        {
            AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z2, x1, y1, z1, x2, y1, z1, x2, y1, z2,
                    uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::east)
        {
            AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y1, z1, x2, y2, z1, x2, y2, z2, x2, y1, z2,
                    uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::west)
        {
            AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z2, x1, y2, z2, x1, y2, z1, x1, y1, z1,
                    uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }

        return;
    }

    if (direction == Direction::north)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z1);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z1);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z1);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z1);
        AddFaceSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, x1, y2, z1, x2, y2, z1, x2, y1,
                      z1, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::south)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z2);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z2);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z2);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z2);
        AddFaceSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y1, z2, x2, y2, z2, x1, y2, z2, x1, y1,
                      z2, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::up)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z1);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z2);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z2);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z1);
        AddFaceSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y2, z1, x1, y2, z2, x2, y2, z2, x2, y2,
                      z1, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::down)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z2);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z1);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z1);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z2);
        AddFaceSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z2, x1, y1, z1, x2, y1, z1, x2, y1,
                      z2, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::east)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z1);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z1);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z2);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z2);
        AddFaceSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y1, z1, x2, y2, z1, x2, y2, z2, x2, y1,
                      z2, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::west)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z2);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z2);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z1);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z1);
        AddFaceSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z2, x1, y2, z2, x1, y2, z1, x1, y1,
                      z1, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
}

void ChunkMesherFaceEmitter::EmitFace4(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect,
                                       uint16_t rawLight, uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2, float x3,
                                       float y3, float z3, float x4, float y4, float z4)
{
    if (!texture)
    {
        return;
    }

    float shadeMul = context->smoothLighting ? GetDirectionalShade(direction) : GetOldDirectionalShade(direction);
    float r;
    float g;
    float b;
    uint32_t tintForLighting = GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
    ChunkMesherLighting::ColorFromRawLight(context->level, rawLight, tintForLighting, shadeMul, &r, &g, &b);

    MeshBucket &bucket = context->bucketLookup->Get(texture);
    AddFace(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, 1.0f,
            1.0f, r, g, b, rawLight, shadeMul, tint, atlasRect);
    // The back face flips U so the texture is unmirrored from behind
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, 0.0f, 1.0f, r, g, b, ToVertexLight(rawLight),
         shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x4, y4, z4, 1.0f, 1.0f, r, g, b, ToVertexLight(rawLight),
         shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x3, y3, z3, 1.0f, 0.0f, r, g, b, ToVertexLight(rawLight),
         shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x3, y3, z3, 1.0f, 0.0f, r, g, b, ToVertexLight(rawLight),
         shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y2, z2, 0.0f, 0.0f, r, g, b, ToVertexLight(rawLight),
         shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, 0.0f, 1.0f, r, g, b, ToVertexLight(rawLight),
         shadeMul, tint, atlasRect);
}

void ChunkMesherFaceEmitter::EmitFixedUVFace(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect,
                                             uint16_t rawLight, uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2, float u0,
                                             float v0, float u1, float v1)
{
    if (!texture)
    {
        return;
    }

    float shadeMul = context->smoothLighting ? GetDirectionalShade(direction) : GetOldDirectionalShade(direction);

    MeshBucket &bucket = context->bucketLookup->Get(texture);
    float r;
    float g;
    float b;
    uint32_t tintForLighting = GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
    ChunkMesherLighting::ColorFromRawLight(context->level, rawLight, tintForLighting, shadeMul, &r, &g, &b);

    if (direction == Direction::north)
    {
        AddFaceUV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, x1, y2, z1, x2, y2, z1, x2, y1, z1,
                  u0, v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
    }
    else if (direction == Direction::south)
    {
        AddFaceUV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y1, z2, x2, y2, z2, x1, y2, z2, x1, y1, z2,
                  u0, v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
    }
    else if (direction == Direction::up)
    {
        AddFaceUV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y2, z1, x1, y2, z2, x2, y2, z2, x2, y2, z1,
                  u0, v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
    }
    else if (direction == Direction::down)
    {
        AddFaceUV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z2, x1, y1, z1, x2, y1, z1, x2, y1, z2,
                  u0, v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
    }
    else if (direction == Direction::east)
    {
        AddFaceUV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x2, y1, z1, x2, y2, z1, x2, y2, z2, x2, y1, z2,
                  u0, v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
    }
    else if (direction == Direction::west)
    {
        AddFaceUV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z2, x1, y2, z2, x1, y2, z1, x1, y1, z1,
                  u0, v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
    }
}

void ChunkMesherFaceEmitter::EmitFace4FixedUV(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect,
                                              uint16_t rawLight, uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2, float x3,
                                              float y3, float z3, float x4, float y4, float z4, float u0, float v0, float u1, float v1)
{
    if (!texture)
    {
        return;
    }

    float shadeMul = context->smoothLighting ? GetDirectionalShade(direction) : GetOldDirectionalShade(direction);
    float r;
    float g;
    float b;
    uint32_t tintForLighting = GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
    ChunkMesherLighting::ColorFromRawLight(context->level, rawLight, tintForLighting, shadeMul, &r, &g, &b);

    MeshBucket &bucket = context->bucketLookup->Get(texture);
    AddFace4UV(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, u0,
               v0, u1, v1, r, g, b, rawLight, shadeMul, tint, atlasRect);
}

void ChunkMesherFaceEmitter::EmitFace4PerVertexUV(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect,
                                                  uint16_t rawLight, uint32_t tint, const FaceVertex *vertices)
{
    float shadeMul = GetOldDirectionalShade(direction);
    float r;
    float g;
    float b;
    uint32_t tintForLighting = GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
    ChunkMesherLighting::ColorFromRawLight(context->level, rawLight, tintForLighting, shadeMul, &r, &g, &b);

    MeshBucket &bucket = context->bucketLookup->Get(texture);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, vertices[0].x, vertices[0].y, vertices[0].z,
         vertices[0].u, vertices[0].v, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, vertices[1].x, vertices[1].y, vertices[1].z,
         vertices[1].u, vertices[1].v, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, vertices[2].x, vertices[2].y, vertices[2].z,
         vertices[2].u, vertices[2].v, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, vertices[2].x, vertices[2].y, vertices[2].z,
         vertices[2].u, vertices[2].v, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, vertices[3].x, vertices[3].y, vertices[3].z,
         vertices[3].u, vertices[3].v, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, vertices[0].x, vertices[0].y, vertices[0].z,
         vertices[0].u, vertices[0].v, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
}

void ChunkMesherFaceEmitter::EmitGreedyFaceRects(FaceEmitContext *context, const std::vector<GreedyRect> &rects, Direction *direction, int plane,
                                                 int yOffset)
{
    for (auto rectIt = rects.begin(); rectIt != rects.end(); rectIt++)
    {
        const GreedyRect &rect = *rectIt;
        float x1;
        float y1;
        float z1;
        float x2;
        float y2;
        float z2;

        if (direction == Direction::east || direction == Direction::west)
        {
            x1 = (float) plane;
            x2 = (float) plane;
            y1 = (float) (rect.y + yOffset);
            y2 = (float) (rect.y + yOffset + rect.height);
            z1 = (float) rect.x;
            z2 = (float) (rect.x + rect.width);
        }
        else if (direction == Direction::north || direction == Direction::south)
        {
            x1 = (float) rect.x;
            x2 = (float) (rect.x + rect.width);
            y1 = (float) (rect.y + yOffset);
            y2 = (float) (rect.y + yOffset + rect.height);
            z1 = (float) plane;
            z2 = (float) plane;
        }
        else
        {
            x1 = (float) rect.x;
            x2 = (float) (rect.x + rect.width);
            y1 = (float) plane;
            y2 = (float) plane;
            z1 = (float) rect.y;
            z2 = (float) (rect.y + rect.height);
        }

        EmitFace(context, direction, rect.texture, rect.atlasRect, rect.rawLight, rect.tint, x1, y1, z1, x2, y2, z2);
    }
}

void ChunkMesherFaceEmitter::EmitBinaryFace(BinaryFaceEmitContext *context, MeshBucket *bucket, Direction *direction, const Block::UVRect &atlasRect,
                                            uint16_t rawLight, uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2)
{
    float uMax = 1.0f;
    float vMax = 1.0f;
    if (direction == Direction::north || direction == Direction::south)
    {
        uMax = fabsf(x2 - x1);
        vMax = fabsf(y2 - y1);
    }
    else if (direction == Direction::east || direction == Direction::west)
    {
        uMax = fabsf(z2 - z1);
        vMax = fabsf(y2 - y1);
    }
    else
    {
        uMax = fabsf(x2 - x1);
        vMax = fabsf(z2 - z1);
    }

    float shadeMul = context->smoothLighting ? GetDirectionalShade(direction) : GetOldDirectionalShade(direction);

    if (!context->smoothLighting)
    {
        float r;
        float g;
        float b;
        uint32_t tintForLighting = GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
        ChunkMesherLighting::ColorFromRawLight(context->level, rawLight, tintForLighting, shadeMul, &r, &g, &b);

        if (direction == Direction::north)
        {
            AddFace(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y1, z1, x1, y2, z1, x2, y2, z1, x2,
                    y1, z1, uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::south)
        {
            AddFace(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x2, y1, z2, x2, y2, z2, x1, y2, z2, x1,
                    y1, z2, uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::up)
        {
            AddFace(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y2, z1, x1, y2, z2, x2, y2, z2, x2,
                    y2, z1, uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::down)
        {
            AddFace(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y1, z2, x1, y1, z1, x2, y1, z1, x2,
                    y1, z2, uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::east)
        {
            AddFace(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x2, y1, z1, x2, y2, z1, x2, y2, z2, x2,
                    y1, z2, uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }
        else if (direction == Direction::west)
        {
            AddFace(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y1, z2, x1, y2, z2, x1, y2, z1, x1,
                    y1, z1, uMax, vMax, r, g, b, rawLight, shadeMul, tint, atlasRect);
        }

        return;
    }

    if (direction == Direction::north)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z1);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z1);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z1);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z1);
        AddFaceSmooth(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y1, z1, x1, y2, z1, x2, y2, z1, x2,
                      y1, z1, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::south)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z2);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z2);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z2);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z2);
        AddFaceSmooth(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x2, y1, z2, x2, y2, z2, x1, y2, z2, x1,
                      y1, z2, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::up)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z1);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z2);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z2);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z1);
        AddFaceSmooth(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y2, z1, x1, y2, z2, x2, y2, z2, x2,
                      y2, z1, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::down)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z2);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z1);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z1);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z2);
        AddFaceSmooth(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y1, z2, x1, y1, z1, x2, y1, z1, x2,
                      y1, z2, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::east)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z1);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z1);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y2, z2);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x2, y1, z2);
        AddFaceSmooth(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x2, y1, z1, x2, y2, z1, x2, y2, z2, x2,
                      y1, z2, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
    else if (direction == Direction::west)
    {
        VertexLight l1 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z2);
        VertexLight l2 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z2);
        VertexLight l3 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y2, z1);
        VertexLight l4 = ChunkMesherCache::BuildVertexLight(*context->buildData, direction, true, rawLight, tint, x1, y1, z1);
        AddFaceSmooth(bucket->vertices, bucket->rawLights, bucket->atlasRects, bucket->shades, bucket->tints, x1, y1, z2, x1, y2, z2, x1, y2, z1, x1,
                      y1, z1, uMax, vMax, l1, l2, l3, l4, tint, atlasRect);
    }
}

void ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(BinaryFaceEmitContext *context, std::unordered_map<Texture *, MeshBucket> *buckets,
                                                       const std::vector<GreedyRect> &rects, Direction *direction, int plane, int yOffset)
{
    for (auto rectIt = rects.begin(); rectIt != rects.end(); rectIt++)
    {
        const GreedyRect &rect = *rectIt;
        MeshBucket *bucket     = ChunkMesherBinaryBuilder::GetBinaryBucket(buckets, rect.texture);
        if (!bucket)
        {
            continue;
        }

        float x1;
        float y1;
        float z1;
        float x2;
        float y2;
        float z2;

        if (direction == Direction::east || direction == Direction::west)
        {
            x1 = (float) plane;
            x2 = (float) plane;
            y1 = (float) (rect.y + yOffset);
            y2 = (float) (rect.y + yOffset + rect.height);
            z1 = (float) rect.x;
            z2 = (float) (rect.x + rect.width);
        }
        else if (direction == Direction::north || direction == Direction::south)
        {
            x1 = (float) rect.x;
            x2 = (float) (rect.x + rect.width);
            y1 = (float) (rect.y + yOffset);
            y2 = (float) (rect.y + yOffset + rect.height);
            z1 = (float) plane;
            z2 = (float) plane;
        }
        else
        {
            x1 = (float) rect.x;
            x2 = (float) (rect.x + rect.width);
            y1 = (float) plane;
            y2 = (float) plane;
            z1 = (float) rect.y;
            z2 = (float) (rect.y + rect.height);
        }

        EmitBinaryFace(context, bucket, direction, rect.atlasRect, rect.rawLight, rect.tint, x1, y1, z1, x2, y2, z2);
    }
}

void ChunkMesherFaceEmitter::AddFace(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                     std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2,
                                     float z2, float x3, float y3, float z3, float x4, float y4, float z4, float uMax, float vMax, float r, float g,
                                     float b, uint16_t rawLight, float shadeMul, uint32_t tint, const Block::UVRect &atlasRect)
{
    Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, uMax, vMax, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x2, y2, z2, uMax, 0.0f, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, 0.0f, 0.0f, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, 0.0f, 0.0f, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x4, y4, z4, 0.0f, vMax, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, uMax, vMax, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
}

void ChunkMesherFaceEmitter::AddFaceUV(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                       std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2,
                                       float z2, float x3, float y3, float z3, float x4, float y4, float z4, float u0, float v0, float u1, float v1,
                                       float r, float g, float b, uint16_t rawLight, float shadeMul, uint32_t tint, const Block::UVRect &atlasRect)
{
    Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, u1, v1, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x2, y2, z2, u1, v0, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, u0, v0, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, u0, v0, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x4, y4, z4, u0, v1, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, u1, v1, r, g, b, ToVertexLight(rawLight), shadeMul, tint, atlasRect);
}

void ChunkMesherFaceEmitter::AddFace4UV(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                        std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2,
                                        float z2, float x3, float y3, float z3, float x4, float y4, float z4, float u0, float v0, float u1, float v1,
                                        float r, float g, float b, uint16_t rawLight, float shadeMul, uint32_t tint, const Block::UVRect &atlasRect)
{
    AddFaceUV(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, u0, v0, u1, v1, r, g, b, rawLight,
              shadeMul, tint, atlasRect);
}

void ChunkMesherFaceEmitter::AddFaceSmooth(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                           std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2,
                                           float z2, float x3, float y3, float z3, float x4, float y4, float z4, float uMax, float vMax,
                                           const VertexLight &l1, const VertexLight &l2, const VertexLight &l3, const VertexLight &l4, uint32_t tint,
                                           const Block::UVRect &atlasRect)
{
    AddFaceUVSmooth(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, 0.0f, 0.0f, uMax, vMax, l1, l2,
                    l3, l4, tint, atlasRect);
}

void ChunkMesherFaceEmitter::AddFaceUVSmooth(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                             std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2,
                                             float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, float u0, float v0,
                                             float u1, float v1, const VertexLight &l1, const VertexLight &l2, const VertexLight &l3,
                                             const VertexLight &l4, uint32_t tint, const Block::UVRect &atlasRect)
{
    float brightness1 = l1.r + l1.g + l1.b;
    float brightness2 = l2.r + l2.g + l2.b;
    float brightness3 = l3.r + l3.g + l3.b;
    float brightness4 = l4.r + l4.g + l4.b;

    if (brightness1 + brightness3 < brightness2 + brightness4)
    {
        Push(vertices, rawLights, atlasRects, shades, tints, x2, y2, z2, u1, v0, l2.r, l2.g, l2.b, l2.vertexLight, l2.shade, tint, atlasRect);
        Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, u0, v0, l3.r, l3.g, l3.b, l3.vertexLight, l3.shade, tint, atlasRect);
        Push(vertices, rawLights, atlasRects, shades, tints, x4, y4, z4, u0, v1, l4.r, l4.g, l4.b, l4.vertexLight, l4.shade, tint, atlasRect);
        Push(vertices, rawLights, atlasRects, shades, tints, x4, y4, z4, u0, v1, l4.r, l4.g, l4.b, l4.vertexLight, l4.shade, tint, atlasRect);
        Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, u1, v1, l1.r, l1.g, l1.b, l1.vertexLight, l1.shade, tint, atlasRect);
        Push(vertices, rawLights, atlasRects, shades, tints, x2, y2, z2, u1, v0, l2.r, l2.g, l2.b, l2.vertexLight, l2.shade, tint, atlasRect);
        return;
    }

    Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, u1, v1, l1.r, l1.g, l1.b, l1.vertexLight, l1.shade, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x2, y2, z2, u1, v0, l2.r, l2.g, l2.b, l2.vertexLight, l2.shade, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, u0, v0, l3.r, l3.g, l3.b, l3.vertexLight, l3.shade, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x3, y3, z3, u0, v0, l3.r, l3.g, l3.b, l3.vertexLight, l3.shade, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x4, y4, z4, u0, v1, l4.r, l4.g, l4.b, l4.vertexLight, l4.shade, tint, atlasRect);
    Push(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, u1, v1, l1.r, l1.g, l1.b, l1.vertexLight, l1.shade, tint, atlasRect);
}

void ChunkMesherFaceEmitter::AddFace4UVSmooth(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                              std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2,
                                              float y2, float z2, float x3, float y3, float z3, float x4, float y4, float z4, float u0, float v0,
                                              float u1, float v1, const VertexLight &l1, const VertexLight &l2, const VertexLight &l3,
                                              const VertexLight &l4, uint32_t tint, const Block::UVRect &atlasRect)
{
    AddFaceUVSmooth(vertices, rawLights, atlasRects, shades, tints, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, u0, v0, u1, v1, l1, l2, l3, l4,
                    tint, atlasRect);
}

#endif
