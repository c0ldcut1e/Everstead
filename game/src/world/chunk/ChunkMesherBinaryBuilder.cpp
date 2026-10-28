#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherBinaryBuilder.h"
#include "ChunkMesherCache.h"
#include "ChunkMesherFaceEmitter.h"
#include "ChunkMesherGreedyMesher.h"
#include "ChunkMesherLighting.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "../../utils/math/Mth.h"
#include "../LevelSource.h"
#include "../lighting/LightEngine.h"

bool ChunkMesherBinaryBuilder::BuildBinaryChunkMeshes(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting,
                                                      bool grassSideOverlay, std::vector<MeshBuildResult> *outMeshes)
{
    if (!level || !chunk || !outMeshes)
    {
        return false;
    }

    const ChunkPos &chunkPos = chunk->GetPos();
    int baseX                = chunkPos.x * Chunk::s_sizeX;
    int baseY                = 0;
    int baseZ                = chunkPos.z * Chunk::s_sizeZ;
    int sectionHeight        = sectionMaxY - sectionMinY;
    Block *grassBlock        = Block::ByName("grass_block");

    thread_local BuildData buildData;
    buildData.Reset(level, chunk, smoothLighting, sectionMinY, sectionMaxY);
    ChunkMesherCache::BuildSolidCache(&buildData);
    if (smoothLighting)
    {
        ChunkMesherCache::BuildRawLightCache(&buildData);
    }

    BinaryFaceEmitContext binaryEmitContext;
    binaryEmitContext.level          = level;
    binaryEmitContext.buildData      = &buildData;
    binaryEmitContext.baseX          = baseX;
    binaryEmitContext.baseY          = baseY;
    binaryEmitContext.baseZ          = baseZ;
    binaryEmitContext.smoothLighting = smoothLighting;

    const uint32_t xMask = (uint32_t) ((1u << Chunk::s_sizeX) - 1u);
    const uint32_t zMask = (uint32_t) ((1u << Chunk::s_sizeZ) - 1u);

    thread_local std::vector<uint32_t> rowBitsX;
    rowBitsX.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeZ, 0);
    thread_local std::vector<uint32_t> rowBitsZ;
    rowBitsZ.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeX, 0);
    // Blocks like grass mesh through the regular path but still hide the faces of the cubes next to them
    thread_local std::vector<uint32_t> occludeBitsX;
    occludeBitsX.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeZ, 0);
    thread_local std::vector<uint32_t> occludeBitsZ;
    occludeBitsZ.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeX, 0);

    int bitsLo = sectionMinY - 1;
    if (bitsLo < 0)
    {
        bitsLo = 0;
    }

    int bitsHi = sectionMaxY + 1;
    if (bitsHi > Chunk::s_sizeY)
    {
        bitsHi = Chunk::s_sizeY;
    }

    for (int y = bitsLo; y < bitsHi; y++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            uint32_t bits        = 0;
            uint32_t occludeBits = 0;
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                Block *block = Block::ById(chunk->GetBlockId(x, y, z));
                if (IsBinaryRenderableBlock(block))
                {
                    bits |= (uint32_t) 1u << x;
                    rowBitsZ[(size_t) y * (size_t) Chunk::s_sizeX + (size_t) x] |= (uint32_t) 1u << z;
                }

                if (IsBinaryOccludingBlock(block))
                {
                    occludeBits |= (uint32_t) 1u << x;
                    occludeBitsZ[(size_t) y * (size_t) Chunk::s_sizeX + (size_t) x] |= (uint32_t) 1u << z;
                }
            }

            rowBitsX[(size_t) y * (size_t) Chunk::s_sizeZ + (size_t) z]     = bits;
            occludeBitsX[(size_t) y * (size_t) Chunk::s_sizeZ + (size_t) z] = occludeBits;
        }
    }

    thread_local std::vector<uint32_t> eastRows;
    eastRows.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeZ, 0);
    thread_local std::vector<uint32_t> westRows;
    westRows.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeZ, 0);
    for (int y = sectionMinY; y < sectionMaxY; y++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            size_t rowIndex = (size_t) y * (size_t) Chunk::s_sizeZ + (size_t) z;
            uint32_t bits   = rowBitsX[rowIndex];
            uint32_t east   = bits & ~(occludeBitsX[rowIndex] >> 1);
            uint32_t west   = bits & ~(occludeBitsX[rowIndex] << 1);
            west &= xMask;

            uint32_t edgeBit = (uint32_t) 1u << (Chunk::s_sizeX - 1);
            if ((east & edgeBit) != 0 && IsBinaryOccludingAt(buildData, baseX + Chunk::s_sizeX, baseY + y, baseZ + z))
            {
                east &= ~edgeBit;
            }

            if ((west & 1u) != 0 && IsBinaryOccludingAt(buildData, baseX - 1, baseY + y, baseZ + z))
            {
                west &= ~1u;
            }

            eastRows[rowIndex] = east & xMask;
            westRows[rowIndex] = west & xMask;
        }
    }

    thread_local std::vector<uint32_t> southRows;
    southRows.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeX, 0);
    thread_local std::vector<uint32_t> northRows;
    northRows.assign((size_t) Chunk::s_sizeY * (size_t) Chunk::s_sizeX, 0);
    for (int y = sectionMinY; y < sectionMaxY; y++)
    {
        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            size_t rowIndex = (size_t) y * (size_t) Chunk::s_sizeX + (size_t) x;
            uint32_t bits   = rowBitsZ[rowIndex];
            uint32_t south  = bits & ~(occludeBitsZ[rowIndex] >> 1);
            uint32_t north  = bits & ~(occludeBitsZ[rowIndex] << 1);
            north &= zMask;

            uint32_t edgeBit = (uint32_t) 1u << (Chunk::s_sizeZ - 1);
            if ((south & edgeBit) != 0 && IsBinaryOccludingAt(buildData, baseX + x, baseY + y, baseZ + Chunk::s_sizeZ))
            {
                south &= ~edgeBit;
            }

            if ((north & 1u) != 0 && IsBinaryOccludingAt(buildData, baseX + x, baseY + y, baseZ - 1))
            {
                north &= ~1u;
            }

            southRows[rowIndex] = south & zMask;
            northRows[rowIndex] = north & zMask;
        }
    }

    std::unordered_map<Texture *, MeshBucket> buckets;
    // These persist per thread, so every use has to assign or clear them first
    thread_local std::vector<MaskCell> mask;
    thread_local std::vector<GreedyRect> greedyRects;
    bool useBinaryGreedy = true;

    mask.assign((size_t) Chunk::s_sizeZ * (size_t) sectionHeight, MaskCell());
    for (int xPlane = 1; xPlane <= Chunk::s_sizeX; xPlane++)
    {
        for (int y = sectionMinY; y < sectionMaxY; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                MaskCell &cell      = mask[(size_t) z + (size_t) Chunk::s_sizeZ * (size_t) (y - sectionMinY)];
                cell.filled         = false;
                cell.texture        = nullptr;
                cell.atlasRect      = {0.0f, 0.0f, 1.0f, 1.0f};
                cell.rawLight       = 0;
                cell.smoothLight00  = 0;
                cell.smoothLight01  = 0;
                cell.smoothLight10  = 0;
                cell.smoothLight11  = 0;
                cell.smoothLighting = false;
                cell.tint           = 0xFFFFFF;
                cell.shapeFlags     = 0;

                uint32_t rowBits = eastRows[(size_t) y * (size_t) Chunk::s_sizeZ + (size_t) z];
                uint32_t bit     = (uint32_t) 1u << (xPlane - 1);
                if ((rowBits & bit) == 0)
                {
                    continue;
                }

                int blockX   = xPlane - 1;
                Block *block = Block::ById(OwnedBlockId(buildData, blockX, y, z));
                if (!block)
                {
                    continue;
                }

                Texture *texture = block->GetTexture(Direction::east);
                if (!texture)
                {
                    continue;
                }

                cell.filled    = true;
                cell.texture   = texture;
                cell.atlasRect = block->GetAtlasUVRect(Direction::east);
                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + xPlane, baseY + y, baseZ + z);
                cell.tint      = SampleBinaryFaceTint(buildData, block, grassBlock, grassSideOverlay, Direction::east, blockX, z, y);
                if (block == grassBlock)
                {
                    ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, blockX, y, z, &cell);
                }

                ChunkMesherCache::SetMaskSmoothLights(buildData, &cell, Direction::east, smoothLighting, (float) xPlane, (float) y, (float) z,
                                                      (float) xPlane, (float) (y + 1), (float) (z + 1));
            }
        }

        greedyRects.clear();
        ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeZ, sectionHeight, useBinaryGreedy, &greedyRects);
        ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(&binaryEmitContext, &buckets, greedyRects, Direction::east, xPlane, sectionMinY);
    }

    for (int xPlane = 0; xPlane < Chunk::s_sizeX; xPlane++)
    {
        for (int y = sectionMinY; y < sectionMaxY; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                MaskCell &cell      = mask[(size_t) z + (size_t) Chunk::s_sizeZ * (size_t) (y - sectionMinY)];
                cell.filled         = false;
                cell.texture        = nullptr;
                cell.atlasRect      = {0.0f, 0.0f, 1.0f, 1.0f};
                cell.rawLight       = 0;
                cell.smoothLight00  = 0;
                cell.smoothLight01  = 0;
                cell.smoothLight10  = 0;
                cell.smoothLight11  = 0;
                cell.smoothLighting = false;
                cell.tint           = 0xFFFFFF;
                cell.shapeFlags     = 0;

                uint32_t rowBits = westRows[(size_t) y * (size_t) Chunk::s_sizeZ + (size_t) z];
                uint32_t bit     = (uint32_t) 1u << xPlane;
                if ((rowBits & bit) == 0)
                {
                    continue;
                }

                int blockX   = xPlane;
                Block *block = Block::ById(OwnedBlockId(buildData, blockX, y, z));
                if (!block)
                {
                    continue;
                }

                Texture *texture = block->GetTexture(Direction::west);
                if (!texture)
                {
                    continue;
                }

                cell.filled    = true;
                cell.texture   = texture;
                cell.atlasRect = block->GetAtlasUVRect(Direction::west);
                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + xPlane - 1, baseY + y, baseZ + z);
                cell.tint      = SampleBinaryFaceTint(buildData, block, grassBlock, grassSideOverlay, Direction::west, blockX, z, y);
                if (block == grassBlock)
                {
                    ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, blockX, y, z, &cell);
                }

                ChunkMesherCache::SetMaskSmoothLights(buildData, &cell, Direction::west, smoothLighting, (float) xPlane, (float) y, (float) z,
                                                      (float) xPlane, (float) (y + 1), (float) (z + 1));
            }
        }

        greedyRects.clear();
        ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeZ, sectionHeight, useBinaryGreedy, &greedyRects);
        ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(&binaryEmitContext, &buckets, greedyRects, Direction::west, xPlane, sectionMinY);
    }

    mask.assign((size_t) Chunk::s_sizeX * (size_t) sectionHeight, MaskCell());
    for (int zPlane = 1; zPlane <= Chunk::s_sizeZ; zPlane++)
    {
        for (int y = sectionMinY; y < sectionMaxY; y++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                MaskCell &cell      = mask[(size_t) x + (size_t) Chunk::s_sizeX * (size_t) (y - sectionMinY)];
                cell.filled         = false;
                cell.texture        = nullptr;
                cell.atlasRect      = {0.0f, 0.0f, 1.0f, 1.0f};
                cell.rawLight       = 0;
                cell.smoothLight00  = 0;
                cell.smoothLight01  = 0;
                cell.smoothLight10  = 0;
                cell.smoothLight11  = 0;
                cell.smoothLighting = false;
                cell.tint           = 0xFFFFFF;
                cell.shapeFlags     = 0;

                uint32_t rowBits = southRows[(size_t) y * (size_t) Chunk::s_sizeX + (size_t) x];
                uint32_t bit     = (uint32_t) 1u << (zPlane - 1);
                if ((rowBits & bit) == 0)
                {
                    continue;
                }

                int blockZ   = zPlane - 1;
                Block *block = Block::ById(OwnedBlockId(buildData, x, y, blockZ));
                if (!block)
                {
                    continue;
                }

                Texture *texture = block->GetTexture(Direction::south);
                if (!texture)
                {
                    continue;
                }

                cell.filled    = true;
                cell.texture   = texture;
                cell.atlasRect = block->GetAtlasUVRect(Direction::south);
                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + zPlane);
                cell.tint      = SampleBinaryFaceTint(buildData, block, grassBlock, grassSideOverlay, Direction::south, x, blockZ, y);
                if (block == grassBlock)
                {
                    ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, x, y, blockZ, &cell);
                }

                ChunkMesherCache::SetMaskSmoothLights(buildData, &cell, Direction::south, smoothLighting, (float) x, (float) y, (float) zPlane,
                                                      (float) (x + 1), (float) (y + 1), (float) zPlane);
            }
        }

        greedyRects.clear();
        ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, sectionHeight, useBinaryGreedy, &greedyRects);
        ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(&binaryEmitContext, &buckets, greedyRects, Direction::south, zPlane, sectionMinY);
    }

    for (int zPlane = 0; zPlane < Chunk::s_sizeZ; zPlane++)
    {
        for (int y = sectionMinY; y < sectionMaxY; y++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                MaskCell &cell      = mask[(size_t) x + (size_t) Chunk::s_sizeX * (size_t) (y - sectionMinY)];
                cell.filled         = false;
                cell.texture        = nullptr;
                cell.atlasRect      = {0.0f, 0.0f, 1.0f, 1.0f};
                cell.rawLight       = 0;
                cell.smoothLight00  = 0;
                cell.smoothLight01  = 0;
                cell.smoothLight10  = 0;
                cell.smoothLight11  = 0;
                cell.smoothLighting = false;
                cell.tint           = 0xFFFFFF;
                cell.shapeFlags     = 0;

                uint32_t rowBits = northRows[(size_t) y * (size_t) Chunk::s_sizeX + (size_t) x];
                uint32_t bit     = (uint32_t) 1u << zPlane;
                if ((rowBits & bit) == 0)
                {
                    continue;
                }

                int blockZ   = zPlane;
                Block *block = Block::ById(OwnedBlockId(buildData, x, y, blockZ));
                if (!block)
                {
                    continue;
                }

                Texture *texture = block->GetTexture(Direction::north);
                if (!texture)
                {
                    continue;
                }

                cell.filled    = true;
                cell.texture   = texture;
                cell.atlasRect = block->GetAtlasUVRect(Direction::north);
                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + zPlane - 1);
                cell.tint      = SampleBinaryFaceTint(buildData, block, grassBlock, grassSideOverlay, Direction::north, x, blockZ, y);
                if (block == grassBlock)
                {
                    ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, x, y, blockZ, &cell);
                }

                ChunkMesherCache::SetMaskSmoothLights(buildData, &cell, Direction::north, smoothLighting, (float) x, (float) y, (float) zPlane,
                                                      (float) (x + 1), (float) (y + 1), (float) zPlane);
            }
        }

        greedyRects.clear();
        ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, sectionHeight, useBinaryGreedy, &greedyRects);
        ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(&binaryEmitContext, &buckets, greedyRects, Direction::north, zPlane, sectionMinY);
    }

    mask.assign((size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ, MaskCell());
    for (int yPlane = sectionMinY + 1; yPlane <= sectionMaxY; yPlane++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            uint32_t belowBits = rowBitsX[(size_t) (yPlane - 1) * (size_t) Chunk::s_sizeZ + (size_t) z];
            uint32_t aboveBits = 0;
            if (yPlane < Chunk::s_sizeY)
            {
                aboveBits = occludeBitsX[(size_t) yPlane * (size_t) Chunk::s_sizeZ + (size_t) z];
            }

            uint32_t upBits = belowBits & ~aboveBits;

            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                MaskCell &cell      = mask[(size_t) x + (size_t) Chunk::s_sizeX * (size_t) z];
                cell.filled         = false;
                cell.texture        = nullptr;
                cell.atlasRect      = {0.0f, 0.0f, 1.0f, 1.0f};
                cell.rawLight       = 0;
                cell.smoothLight00  = 0;
                cell.smoothLight01  = 0;
                cell.smoothLight10  = 0;
                cell.smoothLight11  = 0;
                cell.smoothLighting = false;
                cell.tint           = 0xFFFFFF;
                cell.shapeFlags     = 0;

                uint32_t bit = (uint32_t) 1u << x;
                if ((upBits & bit) == 0)
                {
                    continue;
                }

                int blockY   = yPlane - 1;
                Block *block = Block::ById(OwnedBlockId(buildData, x, blockY, z));
                if (!block)
                {
                    continue;
                }

                Texture *texture = block->GetTexture(Direction::up);
                if (!texture)
                {
                    continue;
                }

                cell.filled    = true;
                cell.texture   = texture;
                cell.atlasRect = block->GetAtlasUVRect(Direction::up);
                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + yPlane, baseZ + z);
                cell.tint      = SampleBinaryFaceTint(buildData, block, grassBlock, grassSideOverlay, Direction::up, x, z, blockY);
                ChunkMesherCache::SetMaskSmoothLights(buildData, &cell, Direction::up, smoothLighting, (float) x, (float) yPlane, (float) z,
                                                      (float) (x + 1), (float) yPlane, (float) (z + 1));
            }
        }

        greedyRects.clear();
        ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, Chunk::s_sizeZ, useBinaryGreedy, &greedyRects);
        ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(&binaryEmitContext, &buckets, greedyRects, Direction::up, yPlane, 0);
    }

    for (int yPlane = sectionMinY; yPlane < sectionMaxY; yPlane++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            uint32_t belowBits = yPlane > 0 ? occludeBitsX[(size_t) (yPlane - 1) * (size_t) Chunk::s_sizeZ + (size_t) z] : 0;
            uint32_t atBits    = rowBitsX[(size_t) yPlane * (size_t) Chunk::s_sizeZ + (size_t) z];
            uint32_t downBits  = atBits & ~belowBits;

            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                MaskCell &cell      = mask[(size_t) x + (size_t) Chunk::s_sizeX * (size_t) z];
                cell.filled         = false;
                cell.texture        = nullptr;
                cell.atlasRect      = {0.0f, 0.0f, 1.0f, 1.0f};
                cell.rawLight       = 0;
                cell.smoothLight00  = 0;
                cell.smoothLight01  = 0;
                cell.smoothLight10  = 0;
                cell.smoothLight11  = 0;
                cell.smoothLighting = false;
                cell.tint           = 0xFFFFFF;
                cell.shapeFlags     = 0;

                uint32_t bit = (uint32_t) 1u << x;
                if ((downBits & bit) == 0)
                {
                    continue;
                }

                int blockY   = yPlane;
                Block *block = Block::ById(OwnedBlockId(buildData, x, blockY, z));
                if (!block)
                {
                    continue;
                }

                Texture *texture = block->GetTexture(Direction::down);
                if (!texture)
                {
                    continue;
                }

                cell.filled    = true;
                cell.texture   = texture;
                cell.atlasRect = block->GetAtlasUVRect(Direction::down);
                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + yPlane - 1, baseZ + z);
                cell.tint      = SampleBinaryFaceTint(buildData, block, grassBlock, grassSideOverlay, Direction::down, x, z, blockY);
                ChunkMesherCache::SetMaskSmoothLights(buildData, &cell, Direction::down, smoothLighting, (float) x, (float) yPlane, (float) z,
                                                      (float) (x + 1), (float) yPlane, (float) (z + 1));
            }
        }

        greedyRects.clear();
        ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, Chunk::s_sizeZ, useBinaryGreedy, &greedyRects);
        ChunkMesherFaceEmitter::EmitBinaryGreedyFaceRects(&binaryEmitContext, &buckets, greedyRects, Direction::down, yPlane, 0);
    }

    outMeshes->clear();
    outMeshes->reserve(buckets.size());

    for (auto bucketIt = buckets.begin(); bucketIt != buckets.end(); ++bucketIt)
    {
        Texture *texture   = bucketIt->first;
        MeshBucket &bucket = bucketIt->second;
        if (bucket.vertices.empty())
        {
            continue;
        }

        MeshBuildResult result;
        result.texture     = texture;
        result.translucent = false;
        result.sortQuads   = false;
        result.vertices    = std::move(bucket.vertices);
        result.atlasRects  = std::move(bucket.atlasRects);
        result.rawLights   = std::move(bucket.rawLights);
        result.shades      = std::move(bucket.shades);
        result.tints       = std::move(bucket.tints);
        outMeshes->push_back(std::move(result));
    }

    return !outMeshes->empty();
}

bool ChunkMesherBinaryBuilder::IsFullBlockAABB(const AABB &aabb)
{
    const glm::dvec3 &min = aabb.GetMin();
    const glm::dvec3 &max = aabb.GetMax();
    return fabs(min.x) <= 0.000001 && fabs(min.y) <= 0.000001 && fabs(min.z) <= 0.000001 && fabs(max.x - 1.0) <= 0.000001 &&
           fabs(max.y - 1.0) <= 0.000001 && fabs(max.z - 1.0) <= 0.000001;
}

bool ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(const Block *block)
{
    if (!block)
    {
        return false;
    }

    if (!block->IsSolid())
    {
        return false;
    }

    if (block->IsTransparent() || block->IsPillar() || block->HasSecondaryTexture() || !block->GetTextureVariantPaths().empty())
    {
        return false;
    }

    if (block->GetRenderShape() != Block::eRenderShape_CUBE)
    {
        return false;
    }

    if (!IsFullBlockAABB(block->GetAABB()))
    {
        return false;
    }

    return true;
}

bool ChunkMesherBinaryBuilder::IsBinaryOccludingBlock(const Block *block)
{
    return block && block->IsSolid() && !block->IsTransparent() && block->GetRenderShape() == Block::eRenderShape_CUBE &&
           IsFullBlockAABB(block->GetAABB());
}

bool ChunkMesherBinaryBuilder::IsBinarySideDirection(Direction *direction)
{
    return direction == Direction::north || direction == Direction::south || direction == Direction::east || direction == Direction::west;
}

bool ChunkMesherBinaryBuilder::IsBinaryOccludingAt(const BuildData &buildData, int wx, int wy, int wz)
{
    if (wy < 0 || wy >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(wx - buildData.baseX, wy - buildData.baseY, wz - buildData.baseZ));
    if (!block)
    {
        return buildData.level->AreEmptyChunksSolidForMeshing();
    }

    return IsBinaryOccludingBlock(block);
}

uint32_t ChunkMesherBinaryBuilder::SampleBinaryFaceTint(BuildData &buildData, Block *block, Block *grassBlock, bool grassSideOverlay,
                                                        Direction *direction, int localX, int localZ, int localY)
{
    uint32_t tint = ChunkMesherCache::ResolveTint(buildData, block, direction, localX, localZ, localY);
    if (grassSideOverlay && block == grassBlock && IsBinarySideDirection(direction))
    {
        uint32_t foliageTint = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, localX, localZ);
        tint                 = ChunkMesherFaceEmitter::PackTintMode(foliageTint, s_tintModeMasked);
    }

    return tint;
}

ChunkMesherBinaryBuilder::MeshBucket *ChunkMesherBinaryBuilder::GetBinaryBucket(std::unordered_map<Texture *, MeshBucket> *buckets, Texture *texture)
{
    if (!buckets || !texture)
    {
        return nullptr;
    }

    auto it = buckets->find(texture);
    if (it != buckets->end())
    {
        return &it->second;
    }

    auto inserted = buckets->insert(std::make_pair(texture, MeshBucket()));
    return &inserted.first->second;
}

#endif
