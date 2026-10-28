#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherResultBuilder.h"

#include "ChunkMesherFaceEmitter.h"

#include <utility>

void ChunkMesherResultBuilder::Build(std::unordered_map<Texture *, MeshBucket> *buckets, const std::vector<Texture *> &usedTextures,
                                     const std::vector<Block::UVRect> &waterAtlasRects, const std::vector<Block::UVRect> &translucentBlockRects,
                                     std::vector<MeshBuildResult> &binaryMeshes, bool hasBinaryMeshes, std::vector<MeshBuildResult> *outMeshes)
{

    outMeshes->clear();
    outMeshes->reserve(usedTextures.size() * 2);

    const size_t faceVertexCount = 6;
    const size_t vertexStride    = 8;
    const size_t atlasStride     = 4;

    for (auto textureIt = usedTextures.begin(); textureIt != usedTextures.end(); ++textureIt)
    {
        Texture *texture = *textureIt;
        if (!texture)
        {
            continue;
        }

        auto bucketIt = buckets->find(texture);
        if (bucketIt == buckets->end())
        {
            continue;
        }

        if (bucketIt->second.vertices.empty())
        {
            continue;
        }

        MeshBucket &bucket = bucketIt->second;
        size_t vertexCount = bucket.vertices.size() / vertexStride;
        if (vertexCount == 0 || vertexCount != bucket.rawLights.size() || vertexCount != bucket.shades.size() || vertexCount != bucket.tints.size() ||
            vertexCount != bucket.atlasRects.size() / atlasStride)
        {
            continue;
        }

        std::vector<float> opaqueVertices;
        std::vector<float> opaqueAtlasRects;
        std::vector<uint32_t> opaqueRawLights;
        std::vector<float> opaqueShades;
        std::vector<uint32_t> opaqueTints;

        std::vector<float> translucentVertices;
        std::vector<float> translucentAtlasRects;
        std::vector<uint32_t> translucentRawLights;
        std::vector<float> translucentShades;
        std::vector<uint32_t> translucentTints;

        opaqueVertices.reserve(bucket.vertices.size());
        opaqueAtlasRects.reserve(bucket.atlasRects.size());
        opaqueRawLights.reserve(bucket.rawLights.size());
        opaqueShades.reserve(bucket.shades.size());
        opaqueTints.reserve(bucket.tints.size());

        translucentVertices.reserve(bucket.vertices.size() / 4);
        translucentAtlasRects.reserve(bucket.atlasRects.size() / 4);
        translucentRawLights.reserve(bucket.rawLights.size() / 4);
        translucentShades.reserve(bucket.shades.size() / 4);
        translucentTints.reserve(bucket.tints.size() / 4);
        std::vector<float> sortedVertices;
        std::vector<float> sortedAtlasRects;
        std::vector<uint32_t> sortedRawLights;
        std::vector<float> sortedShades;
        std::vector<uint32_t> sortedTints;

        for (size_t vertexIndex = 0; vertexIndex + faceVertexCount <= vertexCount; vertexIndex += faceVertexCount)
        {
            size_t atlasIndex = vertexIndex * atlasStride;
            Block::UVRect rect{bucket.atlasRects[atlasIndex + 0], bucket.atlasRects[atlasIndex + 1], bucket.atlasRects[atlasIndex + 2],
                               bucket.atlasRects[atlasIndex + 3]};
            bool faceIsWater = ChunkMesherFaceEmitter::IsWaterAtlasRect(waterAtlasRects, rect);
            // Water stays in its own mesh so it never gets resorted and the translucent passes only draw what they need
            bool faceIsSorted = faceIsWater && ChunkMesherFaceEmitter::IsWaterAtlasRect(translucentBlockRects, rect);

            std::vector<float> *targetVertices     = faceIsSorted ? &sortedVertices : faceIsWater ? &translucentVertices : &opaqueVertices;
            std::vector<float> *targetAtlasRects   = faceIsSorted ? &sortedAtlasRects : faceIsWater ? &translucentAtlasRects : &opaqueAtlasRects;
            std::vector<uint32_t> *targetRawLights = faceIsSorted ? &sortedRawLights : faceIsWater ? &translucentRawLights : &opaqueRawLights;
            std::vector<float> *targetShades       = faceIsSorted ? &sortedShades : faceIsWater ? &translucentShades : &opaqueShades;
            std::vector<uint32_t> *targetTints     = faceIsSorted ? &sortedTints : faceIsWater ? &translucentTints : &opaqueTints;

            size_t vertexStart = vertexIndex * vertexStride;
            size_t vertexEnd   = (vertexIndex + faceVertexCount) * vertexStride;
            targetVertices->insert(targetVertices->end(), bucket.vertices.begin() + vertexStart, bucket.vertices.begin() + vertexEnd);

            size_t atlasStart = vertexIndex * atlasStride;
            size_t atlasEnd   = (vertexIndex + faceVertexCount) * atlasStride;
            targetAtlasRects->insert(targetAtlasRects->end(), bucket.atlasRects.begin() + atlasStart, bucket.atlasRects.begin() + atlasEnd);

            targetRawLights->insert(targetRawLights->end(), bucket.rawLights.begin() + vertexIndex,
                                    bucket.rawLights.begin() + vertexIndex + faceVertexCount);
            targetShades->insert(targetShades->end(), bucket.shades.begin() + vertexIndex, bucket.shades.begin() + vertexIndex + faceVertexCount);
            targetTints->insert(targetTints->end(), bucket.tints.begin() + vertexIndex, bucket.tints.begin() + vertexIndex + faceVertexCount);
        }

        if (!opaqueVertices.empty())
        {
            MeshBuildResult result;
            result.texture     = texture;
            result.translucent = false;
            result.sortQuads   = false;
            result.vertices    = std::move(opaqueVertices);
            result.atlasRects  = std::move(opaqueAtlasRects);
            result.rawLights   = std::move(opaqueRawLights);
            result.shades      = std::move(opaqueShades);
            result.tints       = std::move(opaqueTints);
            outMeshes->push_back(std::move(result));
        }

        if (!translucentVertices.empty())
        {
            MeshBuildResult result;
            result.texture     = texture;
            result.translucent = true;
            result.sortQuads   = false;
            result.vertices    = std::move(translucentVertices);
            result.atlasRects  = std::move(translucentAtlasRects);
            result.rawLights   = std::move(translucentRawLights);
            result.shades      = std::move(translucentShades);
            result.tints       = std::move(translucentTints);
            outMeshes->push_back(std::move(result));
        }

        if (!sortedVertices.empty())
        {
            MeshBuildResult result;
            result.texture     = texture;
            result.translucent = true;
            result.sortQuads   = true;
            result.vertices    = std::move(sortedVertices);
            result.atlasRects  = std::move(sortedAtlasRects);
            result.rawLights   = std::move(sortedRawLights);
            result.shades      = std::move(sortedShades);
            result.tints       = std::move(sortedTints);
            outMeshes->push_back(std::move(result));
        }
    }

    if (hasBinaryMeshes)
    {
        outMeshes->reserve(outMeshes->size() + binaryMeshes.size());
        for (auto binaryIt = binaryMeshes.begin(); binaryIt != binaryMeshes.end(); ++binaryIt)
        {
            outMeshes->push_back(std::move(*binaryIt));
        }
    }
}

#endif
