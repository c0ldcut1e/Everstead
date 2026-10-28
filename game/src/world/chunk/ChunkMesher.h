#pragma once

#include <bit>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../../rendering/Texture.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "../block/BlockRenderer.h"
#include "Chunk.h"
#include "ChunkMesh.h"

#include <glm/vec3.hpp>

class ChunkMesherBuilder;
class ChunkMesherBinaryBuilder;
class ChunkMesherCache;
class ChunkMesherFaceEmitter;
class ChunkMesherGreedyMesher;
class ChunkMesherLighting;
class ChunkMesherResultBuilder;
class ChunkMesherWater;

class ChunkMesher
{
public:
    static constexpr uint8_t s_tintModeMasked    = 255;
    static constexpr uint8_t s_tintModeSnowyLeaf = 1;

    struct MeshBuildResult
    {
        Texture *texture;
        bool translucent;
        bool sortQuads;
        std::vector<float> vertices;
        std::vector<float> atlasRects;
        std::vector<uint32_t> rawLights;
        std::vector<float> shades;
        std::vector<uint32_t> tints;
    };

    static void BuildChunkMeshes(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting, bool grassSideOverlay,
                                 std::vector<MeshBuildResult> *outMeshes);
    static void BuildSectionMeshes(Level *level, const Chunk *chunk, int sectionIndex, bool smoothLighting, bool grassSideOverlay,
                                   std::vector<MeshBuildResult> *outMeshes);

    static bool s_ambientOcclusionEnabled;
    static bool s_smartLeavesEnabled;
    static bool s_forceFullSkyLight;

private:
    friend class ChunkMesherBuilder;
    friend class ChunkMesherBinaryBuilder;
    friend class ChunkMesherCache;
    friend class ChunkMesherFaceEmitter;
    friend class ChunkMesherGreedyMesher;
    friend class ChunkMesherLighting;
    friend class ChunkMesherResultBuilder;
    friend class ChunkMesherWater;

    struct MaskCell
    {
        Texture *texture;
        Block::UVRect atlasRect;
        uint32_t tint;
        uint64_t smoothLight00;
        uint64_t smoothLight01;
        uint64_t smoothLight10;
        uint64_t smoothLight11;
        uint16_t rawLight;
        uint8_t shapeFlags;
        bool smoothLighting;
        bool filled;
    };

    struct GreedyRect
    {
        int x;
        int y;
        int width;
        int height;
        Texture *texture;
        Block::UVRect atlasRect;
        uint16_t rawLight;
        uint32_t tint;
        uint8_t shapeFlags;
    };

    struct VertexLight
    {
        uint32_t vertexLight;
        float r;
        float g;
        float b;
        float shade;
    };

    struct MeshBucket
    {
        std::vector<float> vertices;
        std::vector<float> atlasRects;
        std::vector<uint32_t> rawLights;
        std::vector<float> shades;
        std::vector<uint32_t> tints;
    };

    struct WallMountedVertex
    {
        float x;
        float y;
        float z;
    };

    struct FaceVertex
    {
        float x;
        float y;
        float z;
        float u;
        float v;
    };

    struct BuildData
    {
        BuildData();
        void Reset(Level *newLevel, const Chunk *newChunk, bool shouldCacheRawLight, int newSectionMinY, int newSectionMaxY);

        Level *level;
        const Chunk *chunk;
        ChunkPos chunkPos;
        int baseX;
        int baseY;
        int baseZ;
        int sectionMinY;
        int sectionMaxY;
        bool cacheRawLight;
        const Chunk *chunks[3][3];
        std::vector<uint16_t> rawLights;
        std::vector<uint8_t> rawLoaded;
        std::vector<uint8_t> solids;
        std::vector<uint32_t> smoothLightSums;
        std::vector<uint8_t> smoothLightMeta;
        mutable uint32_t grassTints[Chunk::s_sizeX * Chunk::s_sizeZ];
        mutable uint32_t foliageTints[Chunk::s_sizeX * Chunk::s_sizeZ];
        mutable bool tintsReady[Chunk::s_sizeX * Chunk::s_sizeZ];
        mutable Biome *columnBiomes[Chunk::s_sizeX * Chunk::s_sizeZ];
    };

    struct BucketLookup
    {
        std::unordered_map<Texture *, MeshBucket> *buckets;
        std::unordered_map<Texture *, uint32_t> *usageEpoch;
        std::vector<Texture *> *usedTextures;
        uint32_t epoch;

        MeshBucket &Get(Texture *texture);
    };

    struct FaceEmitContext
    {
        Level *level;
        BuildData *buildData;
        BucketLookup *bucketLookup;
        std::vector<Block::UVRect> *waterAtlasRects;
        int baseX;
        int baseY;
        int baseZ;
        bool smoothLighting;
    };

    struct BinaryFaceEmitContext
    {
        Level *level;
        BuildData *buildData;
        int baseX;
        int baseY;
        int baseZ;
        bool smoothLighting;
    };

    struct SmoothLightAccumulator
    {
        uint8_t sumBr;
        uint8_t sumBg;
        uint8_t sumBb;
        uint8_t sumSk;
        uint8_t missingCount;
    };

    static constexpr int s_buildCacheX       = Chunk::s_sizeX + 2;
    static constexpr int s_buildCacheY       = Chunk::s_sizeY + 2;
    static constexpr int s_buildCacheZ       = Chunk::s_sizeZ + 2;
    static constexpr int s_buildCacheSize    = s_buildCacheX * s_buildCacheY * s_buildCacheZ;
    static constexpr int s_smoothCacheX      = Chunk::s_sizeX + 1;
    static constexpr int s_smoothCacheY      = Chunk::s_sizeY + 1;
    static constexpr int s_smoothCacheZ      = Chunk::s_sizeZ + 1;
    static constexpr int s_smoothCacheSize   = 6 * s_smoothCacheX * s_smoothCacheY * s_smoothCacheZ;
    static constexpr int s_sectionMeshMargin = 2;

    static uint8_t GetTintMode(uint32_t tintPacked);
    static uint32_t PackTintMode(uint32_t tintRgb, uint8_t mode);
    static float GetDirectionalShade(Direction *direction);
    static float GetOldDirectionalShade(Direction *direction);
    static uint16_t PackLight(uint8_t r, uint8_t g, uint8_t b);
    static void UnpackLight(uint16_t k, uint8_t *r, uint8_t *g, uint8_t *b);
    static uint16_t PackRawLight(uint8_t br, uint8_t bg, uint8_t bb, uint8_t sky);
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
    static Direction *DecodeDirection(uint8_t id);
    static uint16_t SampleLightKey(Level *level, const Chunk *chunk, int wx, int wy, int wz);
    static bool SampleLightKeyIfLoaded(Level *level, const Chunk *chunk, int wx, int wy, int wz, uint16_t *outRawLight);
    static void DecodeFaceBasis(Direction *direction, int *nx, int *ny, int *nz, int *ux, int *uy, int *uz, int *vx, int *vy, int *vz);
    static void ColorFromRawLight(Level *level, uint16_t rawLight, uint32_t tint, float shadeMul, float *r, float *g, float *b);
    static uint16_t MaxRawLight(uint16_t a, uint16_t b);
    static bool SampleRenderRawLightIfLoaded(Level *level, const Chunk *chunk, int wx, int wy, int wz, uint16_t *outRawLight);
    static uint32_t SampleSmoothRawLight(Level *level, const Chunk *chunk, Direction *direction, uint16_t faceRawLight, float x, float y, float z);
    static VertexLight BuildVertexLight(Level *level, const Chunk *chunk, Direction *direction, bool smoothLighting, uint16_t faceRawLight,
                                        uint32_t tint, float x, float y, float z);
    static Block *GetBlockLevel(Level *level, const Chunk *chunk, const BlockPos &pos);
    static Block *GetBlockLevel(const BuildData &buildData, const BlockPos &pos);
    static bool IsWaterLevel(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock);
    static bool IsSolidLevel(Level *level, const Chunk *chunk, const BlockPos &pos);
    static bool IsTransparentLevel(Level *level, const Chunk *chunk, const BlockPos &pos);
    static int GetBuildCacheIndex(int x, int y, int z);
    static bool IsInsideBuildCache(int x, int y, int z);
    static int DirectionToSmoothCacheLayer(Direction *direction);
    static int GetSmoothCacheIndex(int layer, int x, int y, int z);
    static bool IsNearlyIntegral(float value);
    static const Chunk *GetBuildChunk2D(const BuildData &buildData, int *x, int *z);
    static const Chunk *GetBuildChunk3D(const BuildData &buildData, int *x, int *y, int *z);
    static void BuildSolidCache(BuildData *buildData);
    static void BuildRawLightCache(BuildData *buildData);
    static uint32_t OwnedBlockId(const BuildData &buildData, int x, int y, int z);
    static uint16_t SampleLightKey(const BuildData &buildData, int wx, int wy, int wz);
    static bool SampleLightKeyIfLoaded(const BuildData &buildData, int wx, int wy, int wz, uint16_t *outRawLight);
    static bool SampleRenderRawLightIfLoaded(const BuildData &buildData, int wx, int wy, int wz, uint16_t *outRawLight);
    static void AccumulateSmoothRawLightSample(const BuildData &buildData, int wx, int wy, int wz, bool countZeroAsFaceLight,
                                               SmoothLightAccumulator *accumulator);
    static uint32_t ResolveSmoothRawLight(uint32_t packedSums, uint8_t faceLightCount, uint16_t faceRawLight);
    static uint32_t SampleSmoothRawLight(const BuildData &buildData, Direction *direction, uint16_t faceRawLight, float x, float y, float z);
    static bool SampleSmoothRawLightCached(BuildData &buildData, Direction *direction, uint16_t faceRawLight, float x, float y, float z,
                                           uint32_t *outRawLight);
    static uint32_t BlendRawLight(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t faceRawLight);
    static bool IsSolidBlockAt(const BuildData &buildData, int wx, int wy, int wz);
    static bool IsTranslucentAt(const BuildData &buildData, int wx, int wy, int wz);
    static float SampleShadeBrightness(const BuildData &buildData, int wx, int wy, int wz);
    static float SampleSmoothAoShade(const BuildData &buildData, Direction *direction, float x, float y, float z);
    static VertexLight BuildVertexLight(BuildData &buildData, Direction *direction, bool smoothLighting, uint16_t faceRawLight, uint32_t tint,
                                        float x, float y, float z);
    static uint8_t PackSmoothLightByte(float value);
    static uint8_t PackRawLightByte(uint32_t vertexLight, int channel);
    static uint64_t MakeSmoothLightKey(const VertexLight &light);
    static uint8_t GetSmoothLightKeyByte(uint64_t key, int index);
    static uint64_t InterpolateSmoothLightKey(uint64_t key00, uint64_t key10, uint64_t key01, int x, int y, int width, int height);
    static void SetMaskSmoothLights(BuildData &buildData, MaskCell *cell, Direction *direction, bool smoothLighting, float x1, float y1, float z1,
                                    float x2, float y2, float z2);
    static bool CanMergeMaskCells(const MaskCell &cell, const MaskCell &nextCell);
    static bool CanRepresentSmoothLightRect(const std::vector<MaskCell> &mask, int width, int x, int y, int rectWidth, int rectHeight);
    static bool IsSolidLevel(const BuildData &buildData, const BlockPos &pos);
    static bool BuildBinaryChunkMeshes(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting, bool grassSideOverlay,
                                       std::vector<MeshBuildResult> *outMeshes);
    static bool IsBinaryRenderableBlock(const Block *block);
    static bool IsBinaryOccludingBlock(const Block *block);
    static bool IsFullBlockAABB(const AABB &aabb);
    static bool IsBinarySideDirection(Direction *direction);
    static bool IsBinaryOccludingAt(const BuildData &buildData, int wx, int wy, int wz);
    static uint32_t SampleBinaryFaceTint(BuildData &buildData, Block *block, Block *grassBlock, bool grassSideOverlay, Direction *direction,
                                         int localX, int localZ);
    static MeshBucket *GetBinaryBucket(std::unordered_map<Texture *, MeshBucket> *buckets, Texture *texture);

    static void Greedy2D(std::vector<MaskCell> &mask, int width, int height, bool useGreedy, std::vector<GreedyRect> *outRects);
};
