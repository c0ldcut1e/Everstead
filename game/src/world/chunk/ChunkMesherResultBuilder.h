#pragma once

#include "ChunkMesher.h"

class ChunkMesherResultBuilder : private ChunkMesher
{
public:
    static void Build(std::unordered_map<Texture *, MeshBucket> *buckets, const std::vector<Texture *> &usedTextures,
                      const std::vector<Block::UVRect> &waterAtlasRects, const std::vector<Block::UVRect> &translucentBlockRects,
                      std::vector<MeshBuildResult> &binaryMeshes, bool hasBinaryMeshes, std::vector<MeshBuildResult> *outMeshes);
};
