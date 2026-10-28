#pragma once

#include "ChunkMesher.h"

class ChunkMesherBinaryBuilder : private ChunkMesher
{
public:
    static bool BuildBinaryChunkMeshes(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting, bool grassSideOverlay,
                                       std::vector<MeshBuildResult> *outMeshes);
    static bool IsBinaryRenderableBlock(const Block *block);
    static bool IsBinaryOccludingBlock(const Block *block);
    static bool IsFullBlockAABB(const AABB &aabb);
    static bool IsBinarySideDirection(Direction *direction);
    static bool IsBinaryOccludingAt(const BuildData &buildData, int wx, int wy, int wz);
    static uint32_t SampleBinaryFaceTint(BuildData &buildData, Block *block, Block *grassBlock, bool grassSideOverlay, Direction *direction,
                                         int localX, int localZ, int localY);
    static MeshBucket *GetBinaryBucket(std::unordered_map<Texture *, MeshBucket> *buckets, Texture *texture);
};
