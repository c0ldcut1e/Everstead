#pragma once

#include "ChunkMesher.h"

class ChunkMesherCache : private ChunkMesher
{
public:
    static bool IsNearlyIntegral(float value);
    static const Chunk *GetBuildChunk2D(const BuildData &buildData, int *x, int *z);
    static const Chunk *GetBuildChunk3D(const BuildData &buildData, int *x, int *y, int *z);
    static void BuildSolidCache(BuildData *buildData);
    static void BuildRawLightCache(BuildData *buildData);
    static uint32_t OwnedBlockId(const BuildData &buildData, int x, int y, int z);
    static uint16_t SampleLightKey(const BuildData &buildData, int wx, int wy, int wz);
    static bool SampleLightKeyIfLoaded(const BuildData &buildData, int wx, int wy, int wz, uint16_t *outRawLight);
    static bool SampleRenderRawLightIfLoaded(const BuildData &buildData, int wx, int wy, int wz, uint16_t *outRawLight);
    static bool IsLightOpaqueAt(const BuildData &buildData, int wx, int wy, int wz);
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
    static void ApplySnowyGrassSide(const BuildData &buildData, const Block *grassBlock, int x, int y, int z, MaskCell *cell);
    static uint32_t ResolveTint(const BuildData &buildData, const Block *block, Direction *direction, int x, int z, int y = -1);

private:
    static void BlendColumnTints(const BuildData &buildData, int x, int z);
};
