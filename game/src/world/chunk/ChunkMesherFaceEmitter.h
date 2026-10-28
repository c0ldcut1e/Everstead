#pragma once

#include "ChunkMesher.h"

class ChunkMesherFaceEmitter : private ChunkMesher
{
public:
    static uint8_t GetTintMode(uint32_t tintPacked);
    static uint32_t PackTintMode(uint32_t tintRgb, uint8_t mode);
    static float GetDirectionalShade(Direction *direction);
    static float GetOldDirectionalShade(Direction *direction);
    static uint16_t PackLight(uint8_t r, uint8_t g, uint8_t b);
    static void UnpackLight(uint16_t k, uint8_t *r, uint8_t *g, uint8_t *b);
    static uint16_t PackRawLight(uint8_t br, uint8_t bg, uint8_t bb, uint8_t sky);
    static uint32_t ToVertexLight(uint16_t rawLight);
    static uint16_t FromVertexLight(uint32_t vertexLight);
    static uint32_t PackSmoothLightSums(uint8_t br, uint8_t bg, uint8_t bb, uint8_t sky);
    static uint8_t UnpackSmoothLightSum(uint32_t packed, int shift);
    static void Push(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects, std::vector<float> &shades,
                     std::vector<uint32_t> &tints, float x, float y, float z, float u, float v, float r, float g, float b, uint32_t vertexLight,
                     float shadeMul, uint32_t tint, const Block::UVRect &atlasRect);
    static bool SameUVRect(const Block::UVRect &a, const Block::UVRect &b);
    static bool IsWaterAtlasRect(const std::vector<Block::UVRect> &waterAtlasRects, const Block::UVRect &rect);
    static void Shade(uint8_t lr, uint8_t lg, uint8_t lb, float shadeMul, float *r, float *g, float *b);
    static void EmitFace(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect, uint16_t rawLight,
                         uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2);
    static void EmitFace4(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect, uint16_t rawLight,
                          uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4, float y4,
                          float z4);
    static void EmitFixedUVFace(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect, uint16_t rawLight,
                                uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2, float u0, float v0, float u1, float v1);
    static void EmitFace4FixedUV(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect, uint16_t rawLight,
                                 uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float x4,
                                 float y4, float z4, float u0, float v0, float u1, float v1);
    static void EmitFace4PerVertexUV(FaceEmitContext *context, Direction *direction, Texture *texture, const Block::UVRect &atlasRect,
                                     uint16_t rawLight, uint32_t tint, const FaceVertex *vertices);
    static void EmitGreedyFaceRects(FaceEmitContext *context, const std::vector<GreedyRect> &rects, Direction *direction, int plane, int yOffset);
    static void EmitBinaryFace(BinaryFaceEmitContext *context, MeshBucket *bucket, Direction *direction, const Block::UVRect &atlasRect,
                               uint16_t rawLight, uint32_t tint, float x1, float y1, float z1, float x2, float y2, float z2);
    static void EmitBinaryGreedyFaceRects(BinaryFaceEmitContext *context, std::unordered_map<Texture *, MeshBucket> *buckets,
                                          const std::vector<GreedyRect> &rects, Direction *direction, int plane, int yOffset);
    static void AddFace(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects, std::vector<float> &shades,
                        std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                        float x4, float y4, float z4, float uMax, float vMax, float r, float g, float b, uint16_t rawLight, float shadeMul,
                        uint32_t tint, const Block::UVRect &atlasRect);
    static void AddFaceUV(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects, std::vector<float> &shades,
                          std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                          float x4, float y4, float z4, float u0, float v0, float u1, float v1, float r, float g, float b, uint16_t rawLight,
                          float shadeMul, uint32_t tint, const Block::UVRect &atlasRect);
    static void AddFace4UV(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects, std::vector<float> &shades,
                           std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3,
                           float x4, float y4, float z4, float u0, float v0, float u1, float v1, float r, float g, float b, uint16_t rawLight,
                           float shadeMul, uint32_t tint, const Block::UVRect &atlasRect);
    static void AddFaceSmooth(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                              std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2, float z2,
                              float x3, float y3, float z3, float x4, float y4, float z4, float uMax, float vMax, const VertexLight &l1,
                              const VertexLight &l2, const VertexLight &l3, const VertexLight &l4, uint32_t tint, const Block::UVRect &atlasRect);
    static void AddFaceUVSmooth(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2, float z2,
                                float x3, float y3, float z3, float x4, float y4, float z4, float u0, float v0, float u1, float v1,
                                const VertexLight &l1, const VertexLight &l2, const VertexLight &l3, const VertexLight &l4, uint32_t tint,
                                const Block::UVRect &atlasRect);
    static void AddFace4UVSmooth(std::vector<float> &vertices, std::vector<uint32_t> &rawLights, std::vector<float> &atlasRects,
                                 std::vector<float> &shades, std::vector<uint32_t> &tints, float x1, float y1, float z1, float x2, float y2, float z2,
                                 float x3, float y3, float z3, float x4, float y4, float z4, float u0, float v0, float u1, float v1,
                                 const VertexLight &l1, const VertexLight &l2, const VertexLight &l3, const VertexLight &l4, uint32_t tint,
                                 const Block::UVRect &atlasRect);
};
