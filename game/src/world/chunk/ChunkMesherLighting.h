#pragma once

#include "ChunkMesher.h"

class ChunkMesherLighting : private ChunkMesher
{
public:
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
    static bool IsUnloadedLevel(const BuildData &buildData, const BlockPos &pos);
    static uint16_t GetBlockDataLevel(const BuildData &buildData, const BlockPos &pos);
    static bool IsWaterLevel(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock);
    static bool IsSolidLevel(Level *level, const Chunk *chunk, const BlockPos &pos);
    static bool IsTransparentLevel(Level *level, const Chunk *chunk, const BlockPos &pos);
    static int GetBuildCacheIndex(int x, int y, int z);
    static bool IsInsideBuildCache(int x, int y, int z);
    static int DirectionToSmoothCacheLayer(Direction *direction);
    static int GetSmoothCacheIndex(int layer, int x, int y, int z);
    static bool IsNearlyIntegral(float value);
};
