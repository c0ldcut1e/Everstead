#pragma once

#include <vector>

#include "ChunkMesher.h"

class ChunkMesherWater : private ChunkMesher
{
public:
    static void Build(const BuildData &buildData, FaceEmitContext *context, int sectionMinY, int sectionMaxY, Block *waterStillBlock,
                      Block *waterFlowBlock);

private:
    struct SurfaceCell
    {
        Texture *texture;
        Block::UVRect atlasRect;
        uint16_t rawLight;
        uint32_t tint;
        float height;
        bool filled;
    };

    static uint16_t GetBlockData(const BuildData &buildData, const BlockPos &pos);
    static bool IsOpen(const Block *block);
    static int GetRenderedDepth(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock);
    static float GetHeight(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock);
    static void AddWeightedHeight(float height, float *total, float *weight);
    static float GetCornerHeight(const BuildData &buildData, float ownHeight, float sideA, float sideB, const BlockPos &diagonal,
                                 Block *waterStillBlock, Block *waterFlowBlock);
    static glm::dvec3 GetFlow(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock);
    static uint16_t SampleFaceLight(const BuildData &buildData, const Block *block, int wx, int wy, int wz);
    static bool CanMerge(const SurfaceCell &cell, const SurfaceCell &nextCell);
};
