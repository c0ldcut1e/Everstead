#pragma once

#include "ChunkMesher.h"

class ChunkMesherBuilder : private ChunkMesher
{
public:
    static void Build(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting, bool grassSideOverlay,
                      std::vector<MeshBuildResult> *outMeshes);

private:
    static constexpr double s_flatBoxThickness = 0.06;

    static bool IsLeafBlock(const Block *block);
    static double Component(const glm::dvec3 &vector, int axis);
    static bool IsBoxFaceCovered(const std::vector<AABB> &boxes, size_t boxIndex, Direction *direction);
    static bool IsFaceCoveredByNeighbor(const AABB &box, const std::vector<AABB> &neighborBoxes, Direction *direction);
    static void EmitBoxesBlock(FaceEmitContext *context, const Block *block, uint16_t data, int x, int y, int z);
    static void EmitTorchPlanes(FaceEmitContext *context, Texture *texture, const Block::UVRect &atlasRect, uint16_t rawLight, uint32_t tint, float x,
                                float y, float z, float dx, float dz);
    static void EmitFireBlock(FaceEmitContext *context, const Block *block, uint16_t rawLight, uint32_t tint, int x, int y, int z);
    static bool IsFireBurnable(const BuildData &buildData, int x, int y, int z);
    static bool IsCulledLeafFace(const BuildData &buildData, const Block *block, const Block *neighbor, const BlockPos &beyond);
    static bool IsCoveredByLava(const BuildData &buildData, const Block *neighbor, const BlockPos &neighborPos, bool upFace);
};
