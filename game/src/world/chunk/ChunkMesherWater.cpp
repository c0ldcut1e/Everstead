#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherWater.h"

#include <cmath>
#include <numbers>

#include "ChunkMesherCache.h"
#include "ChunkMesherFaceEmitter.h"
#include "ChunkMesherLighting.h"

#include <glm/gtx/norm.hpp>

uint16_t ChunkMesherWater::GetBlockData(const BuildData &buildData, const BlockPos &pos)
{
    int x              = pos.x;
    int y              = pos.y;
    int z              = pos.z;
    const Chunk *chunk = ChunkMesherCache::GetBuildChunk3D(buildData, &x, &y, &z);
    return chunk->GetBlockData(x, y, z);
}

bool ChunkMesherWater::IsOpen(const Block *block)
{
    return !block || !block->IsSolid() || block->IsLiquid() || block->IsLiquidReplaceable();
}

int ChunkMesherWater::GetRenderedDepth(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock)
{
    if (!ChunkMesherLighting::IsWaterLevel(buildData, pos, waterStillBlock, waterFlowBlock))
    {
        return -1;
    }

    int depth = GetBlockData(buildData, pos);
    return depth >= 8 ? 0 : depth;
}

float ChunkMesherWater::GetHeight(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock)
{
    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, pos);
    if (BlockRenderer::IsWaterBlock(block, waterStillBlock, waterFlowBlock))
    {
        if (ChunkMesherLighting::IsWaterLevel(buildData, BlockPos(pos.x, pos.y + 1, pos.z), waterStillBlock, waterFlowBlock))
        {
            return 1.0f;
        }

        int depth = GetBlockData(buildData, pos);
        return (float) (8 - (depth >= 8 ? 0 : depth)) / 9.0f;
    }

    // An unloaded neighbor chunk has no block, counting it as open would pull the corners at chunk edges down
    return block && IsOpen(block) ? 0.0f : -1.0f;
}

void ChunkMesherWater::AddWeightedHeight(float height, float *total, float *weight)
{
    if (height >= 0.8f)
    {
        *total += height * 10.0f;
        *weight += 10.0f;
    }
    else if (height >= 0.0f)
    {
        *total += height;
        *weight += 1.0f;
    }
}

float ChunkMesherWater::GetCornerHeight(const BuildData &buildData, float ownHeight, float sideA, float sideB, const BlockPos &diagonal,
                                        Block *waterStillBlock, Block *waterFlowBlock)
{
    if (sideA >= 1.0f || sideB >= 1.0f)
    {
        return 1.0f;
    }

    float total  = 0.0f;
    float weight = 0.0f;
    if (sideA > 0.0f || sideB > 0.0f)
    {
        float diagonalHeight = GetHeight(buildData, diagonal, waterStillBlock, waterFlowBlock);
        if (diagonalHeight >= 1.0f)
        {
            return 1.0f;
        }

        AddWeightedHeight(diagonalHeight, &total, &weight);
    }

    AddWeightedHeight(ownHeight, &total, &weight);
    AddWeightedHeight(sideA, &total, &weight);
    AddWeightedHeight(sideB, &total, &weight);
    return total / weight;
}

glm::dvec3 ChunkMesherWater::GetFlow(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock)
{
    glm::dvec3 flow;
    int middleDepth = GetRenderedDepth(buildData, pos, waterStillBlock, waterFlowBlock);
    for (int direction = 0; direction < 4; direction++)
    {
        BlockPos neighbor = pos;
        if (direction == 0)
        {
            neighbor.x--;
        }
        else if (direction == 1)
        {
            neighbor.z--;
        }
        else if (direction == 2)
        {
            neighbor.x++;
        }
        else
        {
            neighbor.z++;
        }

        int depth = GetRenderedDepth(buildData, neighbor, waterStillBlock, waterFlowBlock);
        if (depth < 0)
        {
            if (IsOpen(ChunkMesherLighting::GetBlockLevel(buildData, neighbor)))
            {
                depth = GetRenderedDepth(buildData, BlockPos(neighbor.x, neighbor.y - 1, neighbor.z), waterStillBlock, waterFlowBlock);
                if (depth >= 0)
                {
                    int difference = depth - (middleDepth - 8);
                    flow.x += (double) (neighbor.x - pos.x) * difference;
                    flow.z += (double) (neighbor.z - pos.z) * difference;
                }
            }
        }
        else
        {
            int difference = depth - middleDepth;
            flow.x += (double) (neighbor.x - pos.x) * difference;
            flow.z += (double) (neighbor.z - pos.z) * difference;
        }
    }

    return glm::length2(flow) > 0.0 ? glm::normalize(flow) : flow;
}

bool ChunkMesherWater::CanMerge(const SurfaceCell &cell, const SurfaceCell &nextCell)
{
    return nextCell.filled && nextCell.texture == cell.texture && ChunkMesherFaceEmitter::SameUVRect(nextCell.atlasRect, cell.atlasRect) &&
           nextCell.rawLight == cell.rawLight && nextCell.tint == cell.tint && nextCell.height == cell.height;
}

uint16_t ChunkMesherWater::SampleFaceLight(const BuildData &buildData, const Block *block, int wx, int wy, int wz)
{
    uint16_t rawLight = ChunkMesherCache::SampleLightKey(buildData, wx, wy, wz);
    if (block->GetLightEmission() > 0)
    {
        return ChunkMesherFaceEmitter::PackRawLight(15, 15, 15, (uint8_t) (rawLight >> 12));
    }

    return rawLight;
}

void ChunkMesherWater::Build(const BuildData &buildData, FaceEmitContext *context, int sectionMinY, int sectionMaxY, Block *waterStillBlock,
                             Block *waterFlowBlock)
{
    const float offset  = 0.001f;
    int sectionHeight   = sectionMaxY - sectionMinY;
    bool smoothLighting = context->smoothLighting;
    if (waterStillBlock->GetLightEmission() > 0)
    {
        context->smoothLighting = false;
    }

    thread_local std::vector<SurfaceCell> surfaces;
    surfaces.clear();
    surfaces.resize((size_t) sectionHeight * (size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ);

    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int y = sectionMinY; y < sectionMaxY; y++)
        {
            for (int z = 0; z < Chunk::s_sizeZ; z++)
            {
                Block *block = Block::ById(buildData.chunk->GetBlockId(x, y, z));
                if (!BlockRenderer::IsWaterBlock(block, waterStillBlock, waterFlowBlock))
                {
                    continue;
                }

                bool up    = !ChunkMesherLighting::IsWaterLevel(buildData, BlockPos(x, y + 1, z), waterStillBlock, waterFlowBlock);
                bool down  = BlockRenderer::TesselateWaterInWorld(block, ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y - 1, z)),
                                                                  waterStillBlock, waterFlowBlock);
                bool north = BlockRenderer::TesselateWaterInWorld(block, ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z - 1)),
                                                                  waterStillBlock, waterFlowBlock) &&
                             !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y, z - 1));
                bool south = BlockRenderer::TesselateWaterInWorld(block, ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z + 1)),
                                                                  waterStillBlock, waterFlowBlock) &&
                             !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y, z + 1));
                bool west  = BlockRenderer::TesselateWaterInWorld(block, ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x - 1, y, z)),
                                                                  waterStillBlock, waterFlowBlock) &&
                             !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x - 1, y, z));
                bool east  = BlockRenderer::TesselateWaterInWorld(block, ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x + 1, y, z)),
                                                                  waterStillBlock, waterFlowBlock) &&
                             !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x + 1, y, z));
                if (!up && !down && !north && !south && !west && !east)
                {
                    continue;
                }

                float heights[4];
                float ownHeight = GetHeight(buildData, BlockPos(x, y, z), waterStillBlock, waterFlowBlock);
                if (ownHeight >= 1.0f)
                {
                    heights[0] = heights[1] = heights[2] = heights[3] = 1.0f;
                }
                else
                {
                    float northHeight = GetHeight(buildData, BlockPos(x, y, z - 1), waterStillBlock, waterFlowBlock);
                    float southHeight = GetHeight(buildData, BlockPos(x, y, z + 1), waterStillBlock, waterFlowBlock);
                    float westHeight  = GetHeight(buildData, BlockPos(x - 1, y, z), waterStillBlock, waterFlowBlock);
                    float eastHeight  = GetHeight(buildData, BlockPos(x + 1, y, z), waterStillBlock, waterFlowBlock);
                    heights[0]        = GetCornerHeight(buildData, ownHeight, northHeight, westHeight, BlockPos(x - 1, y, z - 1), waterStillBlock,
                                                        waterFlowBlock);
                    heights[1]        = GetCornerHeight(buildData, ownHeight, southHeight, westHeight, BlockPos(x - 1, y, z + 1), waterStillBlock,
                                                        waterFlowBlock);
                    heights[2]        = GetCornerHeight(buildData, ownHeight, southHeight, eastHeight, BlockPos(x + 1, y, z + 1), waterStillBlock,
                                                        waterFlowBlock);
                    heights[3]        = GetCornerHeight(buildData, ownHeight, northHeight, eastHeight, BlockPos(x + 1, y, z - 1), waterStillBlock,
                                                        waterFlowBlock);
                }

                uint32_t tint = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z);
                FaceVertex vertices[4];

                if (up)
                {
                    glm::dvec3 flow = GetFlow(buildData, BlockPos(x, y, z), waterStillBlock, waterFlowBlock);
                    Texture *texture;
                    Block::UVRect atlasRect;
                    if (flow.x == 0.0 && flow.z == 0.0)
                    {
                        texture   = waterStillBlock->GetTexture(Direction::up);
                        atlasRect = waterStillBlock->GetAtlasUVRect(Direction::up);
                        if (heights[0] == heights[1] && heights[0] == heights[2] && heights[0] == heights[3])
                        {
                            SurfaceCell &surface = surfaces[(size_t) (y - sectionMinY) * (size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ +
                                                            (size_t) z * (size_t) Chunk::s_sizeX + (size_t) x];
                            surface.texture      = texture;
                            surface.atlasRect    = atlasRect;
                            surface.rawLight     = SampleFaceLight(buildData, block, buildData.baseX + x, y + 1, buildData.baseZ + z);
                            surface.tint         = tint;
                            surface.height       = heights[0];
                            surface.filled       = true;
                            texture              = nullptr;
                        }
                        else
                        {
                            vertices[0] = {(float) x, (float) y + heights[0] - offset, (float) z, 0.0f, 0.0f};
                            vertices[1] = {(float) x, (float) y + heights[1] - offset, (float) z + 1.0f, 0.0f, 1.0f};
                            vertices[2] = {(float) x + 1.0f, (float) y + heights[2] - offset, (float) z + 1.0f, 1.0f, 1.0f};
                            vertices[3] = {(float) x + 1.0f, (float) y + heights[3] - offset, (float) z, 1.0f, 0.0f};
                        }
                    }
                    else
                    {
                        texture      = waterFlowBlock->GetTexture(Direction::up);
                        atlasRect    = waterFlowBlock->GetAtlasUVRect(Direction::up);
                        float angle  = (float) atan2(flow.z, flow.x) - (float) std::numbers::pi / 2.0f;
                        float sine   = sinf(angle) * 0.25f;
                        float cosine = cosf(angle) * 0.25f;
                        vertices[0]  = {(float) x, (float) y + heights[0] - offset, (float) z, 0.5f - cosine - sine, 0.5f - cosine + sine};
                        vertices[1]  = {(float) x, (float) y + heights[1] - offset, (float) z + 1.0f, 0.5f - cosine + sine, 0.5f + cosine + sine};
                        vertices[2]  = {(float) x + 1.0f, (float) y + heights[2] - offset, (float) z + 1.0f, 0.5f + cosine + sine,
                                        0.5f + cosine - sine};
                        vertices[3]  = {(float) x + 1.0f, (float) y + heights[3] - offset, (float) z, 0.5f + cosine - sine, 0.5f - cosine - sine};
                    }

                    if (texture)
                    {
                        uint16_t rawLight = SampleFaceLight(buildData, block, buildData.baseX + x, y + 1, buildData.baseZ + z);
                        ChunkMesherFaceEmitter::EmitFace4PerVertexUV(context, Direction::up, texture, atlasRect, rawLight, tint, vertices);
                        FaceVertex reversed[] = {vertices[3], vertices[2], vertices[1], vertices[0]};
                        ChunkMesherFaceEmitter::EmitFace4PerVertexUV(context, Direction::up, texture, atlasRect, rawLight, tint, reversed);
                    }

                    heights[0] -= offset;
                    heights[1] -= offset;
                    heights[2] -= offset;
                    heights[3] -= offset;
                }

                if (down)
                {
                    ChunkMesherFaceEmitter::EmitFace(
                            context, Direction::down, waterStillBlock->GetTexture(Direction::down), waterStillBlock->GetAtlasUVRect(Direction::down),
                            SampleFaceLight(buildData, block, buildData.baseX + x, y - 1, buildData.baseZ + z), tint, (float) x, (float) y + offset,
                            (float) z, (float) x + 1.0f, (float) y + offset, (float) z + 1.0f);
                }

                if (north)
                {
                    vertices[0] = {(float) x, (float) y + heights[0], (float) z + offset, 0.0f, (1.0f - heights[0]) * 0.5f};
                    vertices[1] = {(float) x + 1.0f, (float) y + heights[3], (float) z + offset, 0.5f, (1.0f - heights[3]) * 0.5f};
                    vertices[2] = {(float) x + 1.0f, (float) y, (float) z + offset, 0.5f, 0.5f};
                    vertices[3] = {(float) x, (float) y, (float) z + offset, 0.0f, 0.5f};
                    ChunkMesherFaceEmitter::EmitFace4PerVertexUV(
                            context, Direction::north, waterFlowBlock->GetTexture(Direction::north), waterFlowBlock->GetAtlasUVRect(Direction::north),
                            SampleFaceLight(buildData, block, buildData.baseX + x, y, buildData.baseZ + z - 1), tint, vertices);
                }

                if (south)
                {
                    vertices[0] = {(float) x + 1.0f, (float) y + heights[2], (float) z + 1.0f - offset, 0.0f, (1.0f - heights[2]) * 0.5f};
                    vertices[1] = {(float) x, (float) y + heights[1], (float) z + 1.0f - offset, 0.5f, (1.0f - heights[1]) * 0.5f};
                    vertices[2] = {(float) x, (float) y, (float) z + 1.0f - offset, 0.5f, 0.5f};
                    vertices[3] = {(float) x + 1.0f, (float) y, (float) z + 1.0f - offset, 0.0f, 0.5f};
                    ChunkMesherFaceEmitter::EmitFace4PerVertexUV(
                            context, Direction::south, waterFlowBlock->GetTexture(Direction::south), waterFlowBlock->GetAtlasUVRect(Direction::south),
                            SampleFaceLight(buildData, block, buildData.baseX + x, y, buildData.baseZ + z + 1), tint, vertices);
                }

                if (west)
                {
                    vertices[0] = {(float) x + offset, (float) y + heights[1], (float) z + 1.0f, 0.0f, (1.0f - heights[1]) * 0.5f};
                    vertices[1] = {(float) x + offset, (float) y + heights[0], (float) z, 0.5f, (1.0f - heights[0]) * 0.5f};
                    vertices[2] = {(float) x + offset, (float) y, (float) z, 0.5f, 0.5f};
                    vertices[3] = {(float) x + offset, (float) y, (float) z + 1.0f, 0.0f, 0.5f};
                    ChunkMesherFaceEmitter::EmitFace4PerVertexUV(
                            context, Direction::west, waterFlowBlock->GetTexture(Direction::west), waterFlowBlock->GetAtlasUVRect(Direction::west),
                            SampleFaceLight(buildData, block, buildData.baseX + x - 1, y, buildData.baseZ + z), tint, vertices);
                }

                if (east)
                {
                    vertices[0] = {(float) x + 1.0f - offset, (float) y + heights[3], (float) z, 0.0f, (1.0f - heights[3]) * 0.5f};
                    vertices[1] = {(float) x + 1.0f - offset, (float) y + heights[2], (float) z + 1.0f, 0.5f, (1.0f - heights[2]) * 0.5f};
                    vertices[2] = {(float) x + 1.0f - offset, (float) y, (float) z + 1.0f, 0.5f, 0.5f};
                    vertices[3] = {(float) x + 1.0f - offset, (float) y, (float) z, 0.0f, 0.5f};
                    ChunkMesherFaceEmitter::EmitFace4PerVertexUV(
                            context, Direction::east, waterFlowBlock->GetTexture(Direction::east), waterFlowBlock->GetAtlasUVRect(Direction::east),
                            SampleFaceLight(buildData, block, buildData.baseX + x + 1, y, buildData.baseZ + z), tint, vertices);
                }
            }
        }
    }

    for (int y = 0; y < sectionHeight; y++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int x = 0; x < Chunk::s_sizeX; x++)
            {
                SurfaceCell &surface =
                        surfaces[(size_t) y * (size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ + (size_t) z * (size_t) Chunk::s_sizeX + (size_t) x];
                if (!surface.filled)
                {
                    continue;
                }

                int width = 1;
                while (x + width < Chunk::s_sizeX && CanMerge(surface, surfaces[(size_t) y * (size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ +
                                                                                (size_t) z * (size_t) Chunk::s_sizeX + (size_t) (x + width)]))
                {
                    width++;
                }

                int depth = 1;
                bool extend;
                do
                {
                    extend = z + depth < Chunk::s_sizeZ;
                    for (int dx = 0; extend && dx < width; dx++)
                    {
                        extend = CanMerge(surface, surfaces[(size_t) y * (size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ +
                                                            (size_t) (z + depth) * (size_t) Chunk::s_sizeX + (size_t) (x + dx)]);
                    }

                    if (extend)
                    {
                        depth++;
                    }
                } while (extend);

                for (int dz = 0; dz < depth; dz++)
                {
                    for (int dx = 0; dx < width; dx++)
                    {
                        surfaces[(size_t) y * (size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ + (size_t) (z + dz) * (size_t) Chunk::s_sizeX +
                                 (size_t) (x + dx)]
                                .filled = false;
                    }
                }

                float surfaceY = (float) (y + sectionMinY) + surface.height - offset;
                ChunkMesherFaceEmitter::EmitFace(context, Direction::up, surface.texture, surface.atlasRect, surface.rawLight, surface.tint,
                                                 (float) x, surfaceY, (float) z, (float) (x + width), surfaceY, (float) (z + depth));
                ChunkMesherFaceEmitter::EmitFace(context, Direction::down, surface.texture, surface.atlasRect, surface.rawLight, surface.tint,
                                                 (float) x, surfaceY, (float) z, (float) (x + width), surfaceY, (float) (z + depth));
            }
        }
    }

    context->smoothLighting = smoothLighting;
}

#endif
