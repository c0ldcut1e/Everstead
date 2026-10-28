#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherBuilder.h"

#include "ChunkMesherBinaryBuilder.h"
#include "ChunkMesherCache.h"
#include "ChunkMesherFaceEmitter.h"
#include "ChunkMesherGreedyMesher.h"
#include "ChunkMesherLighting.h"
#include "ChunkMesherResultBuilder.h"
#include "ChunkMesherWater.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "../../utils/math/Mth.h"
#include "../LevelSource.h"
#include "../block/AttachedStemBlock.h"
#include "../block/BlockRegistry.h"
#include "../block/CropBlock.h"
#include "../block/DiodeBlock.h"
#include "../block/StemBlock.h"
#include "../lighting/LightEngine.h"

#include <glm/geometric.hpp>

bool ChunkMesherBuilder::IsLeafBlock(const Block *block)
{
    return block && block->IsTransparent() && !block->CullsSameNeighbor();
}

bool ChunkMesherBuilder::IsCulledLeafFace(const BuildData &buildData, const Block *block, const Block *neighbor, const BlockPos &beyond)
{
    if (!ChunkMesher::s_smartLeavesEnabled || !IsLeafBlock(block) || !IsLeafBlock(neighbor) || beyond.y < 0 || beyond.y >= Chunk::s_sizeY)
    {
        return false;
    }

    return IsLeafBlock(ChunkMesherLighting::GetBlockLevel(buildData, beyond));
}

bool ChunkMesherBuilder::IsCoveredByLava(const BuildData &buildData, const Block *neighbor, const BlockPos &neighborPos, bool upFace)
{
    if (!neighbor || !neighbor->IsLava())
    {
        return false;
    }

    if (upFace)
    {
        return true;
    }

    if (neighborPos.y + 1 >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *above = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(neighborPos.x, neighborPos.y + 1, neighborPos.z));
    return above && above->IsLava();
}

double ChunkMesherBuilder::Component(const glm::dvec3 &vector, int axis)
{
    return axis == 0 ? vector.x : (axis == 1 ? vector.y : vector.z);
}

bool ChunkMesherBuilder::IsBoxFaceCovered(const std::vector<AABB> &boxes, size_t boxIndex, Direction *direction)
{
    const AABB &box = boxes[boxIndex];
    int axis        = direction->dx != 0 ? 0 : (direction->dy != 0 ? 1 : 2);
    int sign        = direction->dx + direction->dy + direction->dz;
    double plane    = sign > 0 ? Component(box.GetMax(), axis) : Component(box.GetMin(), axis);
    for (size_t i = 0; i < boxes.size(); i++)
    {
        const AABB &other = boxes[i];
        double otherPlane = sign > 0 ? Component(other.GetMin(), axis) : Component(other.GetMax(), axis);
        if (i == boxIndex || otherPlane != plane)
        {
            continue;
        }

        bool covers = true;
        for (int a = 0; a < 3 && covers; a++)
        {
            covers = a == axis ||
                     (Component(other.GetMin(), a) <= Component(box.GetMin(), a) && Component(other.GetMax(), a) >= Component(box.GetMax(), a));
        }

        if (covers)
        {
            return true;
        }
    }

    return false;
}

bool ChunkMesherBuilder::IsFaceCoveredByNeighbor(const AABB &box, const std::vector<AABB> &neighborBoxes, Direction *direction)
{
    int axis = direction->dx != 0 ? 0 : (direction->dy != 0 ? 1 : 2);
    int sign = direction->dx + direction->dy + direction->dz;
    for (size_t i = 0; i < neighborBoxes.size(); i++)
    {
        const AABB &other = neighborBoxes[i];
        double otherPlane = sign > 0 ? Component(other.GetMin(), axis) : Component(other.GetMax(), axis);
        if (otherPlane != (sign > 0 ? 0.0 : 1.0))
        {
            continue;
        }

        bool covers = true;
        for (int a = 0; a < 3 && covers; a++)
        {
            covers = a == axis ||
                     (Component(other.GetMin(), a) <= Component(box.GetMin(), a) && Component(other.GetMax(), a) >= Component(box.GetMax(), a));
        }

        if (covers)
        {
            return true;
        }
    }

    return false;
}

static std::vector<Block::UVRect> CollectTranslucentBlockRects()
{
    std::vector<Block::UVRect> rects;
    BlockRegistry *registry = BlockRegistry::GetInstance();
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block = registry->ById(i);
        if (!block || !block->IsTranslucent())
        {
            continue;
        }

        rects.push_back(block->GetAtlasUVRect(Direction::north));
        if (block->HasSecondaryTexture())
        {
            rects.push_back(block->GetSecondaryAtlasUVRect());
        }
    }

    return rects;
}

bool ChunkMesherBuilder::IsFireBurnable(const BuildData &buildData, int x, int y, int z)
{
    if (y < 0 || y >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z));
    return block && block->GetFireEncouragement() > 0;
}

// Chunk vertices clamp to the chunk bounds, so the planes only span the opaque middle of the texture to stay inside the block
void ChunkMesherBuilder::EmitTorchPlanes(FaceEmitContext *context, Texture *texture, const Block::UVRect &atlasRect, uint16_t rawLight, uint32_t tint,
                                         float x, float y, float z, float dx, float dz)
{
    float w                = 1.0f / 16.0f;
    float h                = 10.0f / 16.0f;
    float s                = 3.0f / 16.0f;
    float u0               = 0.5f - s;
    float u1               = 0.5f + s;
    float topX             = x + dx * (1.0f - h);
    float topZ             = z + dz * (1.0f - h);
    FaceVertex faces[5][4] = {{{topX - w, y + h, topZ - w, 7.0f / 16.0f, 6.0f / 16.0f},
                               {topX - w, y + h, topZ + w, 7.0f / 16.0f, 8.0f / 16.0f},
                               {topX + w, y + h, topZ + w, 9.0f / 16.0f, 8.0f / 16.0f},
                               {topX + w, y + h, topZ - w, 9.0f / 16.0f, 6.0f / 16.0f}},
                              {{x - w + dx, y, z + s + dz, u1, 1.0f},
                               {x - w, y + 1.0f, z + s, u1, 0.0f},
                               {x - w, y + 1.0f, z - s, u0, 0.0f},
                               {x - w + dx, y, z - s + dz, u0, 1.0f}},
                              {{x + w + dx, y, z - s + dz, u1, 1.0f},
                               {x + w, y + 1.0f, z - s, u1, 0.0f},
                               {x + w, y + 1.0f, z + s, u0, 0.0f},
                               {x + w + dx, y, z + s + dz, u0, 1.0f}},
                              {{x - s + dx, y, z - w + dz, u1, 1.0f},
                               {x - s, y + 1.0f, z - w, u1, 0.0f},
                               {x + s, y + 1.0f, z - w, u0, 0.0f},
                               {x + s + dx, y, z - w + dz, u0, 1.0f}},
                              {{x + s + dx, y, z + w + dz, u1, 1.0f},
                               {x + s, y + 1.0f, z + w, u1, 0.0f},
                               {x - s, y + 1.0f, z + w, u0, 0.0f},
                               {x - s + dx, y, z + w + dz, u0, 1.0f}}};

    // Wall torch bases lean into the wall, so we clip them along an edge to the block bounds with matching UVs
    float minX = floorf(x);
    float minZ = floorf(z);
    for (int i = 1; i < 5; i++)
    {
        for (int bottom = 0; bottom < 4; bottom += 3)
        {
            FaceVertex &vertex = faces[i][bottom];
            float limitX       = std::clamp(vertex.x, minX, minX + 1.0f);
            float limitZ       = std::clamp(vertex.z, minZ, minZ + 1.0f);
            if (limitX == vertex.x && limitZ == vertex.z)
            {
                continue;
            }

            bool alongX               = limitX != vertex.x;
            float position            = alongX ? vertex.x : vertex.z;
            float limit               = alongX ? limitX : limitZ;
            const FaceVertex &partner = faces[i][3 - bottom];
            float partnerPosition     = alongX ? partner.x : partner.z;
            float partnerMin          = alongX ? minX : minZ;
            bool partnerInside        = partnerPosition >= partnerMin && partnerPosition <= partnerMin + 1.0f;
            const FaceVertex &target  = partnerInside ? partner : faces[i][bottom == 0 ? 1 : 2];
            float t                   = (limit - position) / ((alongX ? target.x : target.z) - position);
            vertex.x += (target.x - vertex.x) * t;
            vertex.y += (target.y - vertex.y) * t;
            vertex.z += (target.z - vertex.z) * t;
            vertex.u += (target.u - vertex.u) * t;
            vertex.v += (target.v - vertex.v) * t;
        }
    }

    for (int i = 0; i < 5; i++)
    {
        ChunkMesherFaceEmitter::EmitFace4PerVertexUV(context, Direction::up, texture, atlasRect, rawLight, tint, faces[i]);
    }
}

void ChunkMesherBuilder::EmitFireBlock(FaceEmitContext *context, const Block *block, uint16_t rawLight, uint32_t tint, int x, int y, int z)
{
    const float height  = 1.4f;
    const float sideGap = 0.01f;

    BuildData &buildData = *context->buildData;
    Texture *texture     = block->GetTexture(Direction::up);
    Block::UVRect layers[2];
    layers[0] = block->GetAtlasUVRect(Direction::up);
    layers[1] = block->GetSecondaryAtlasUVRect();

    float x0 = (float) x;
    float y0 = (float) y;
    float z0 = (float) z;
    float x1 = x0 + 1.0f;
    float z1 = z0 + 1.0f;

    Block *below               = y > 0 ? ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y - 1, z)) : nullptr;
    bool floor                 = (below && below->IsSolid()) || IsFireBurnable(buildData, x, y - 1, z);
    uint32_t hash              = (uint32_t) (x * 3129871) ^ (uint32_t) (z * 116129781) ^ (uint32_t) y;
    const Block::UVRect &layer = layers[(hash >> 4) & 1];
    float top                  = y0 + height;
    if (floor)
    {
        const float offset = 0.05f;
        const float lean   = 0.41421356f;
        float bottom       = y0 + offset * lean;
        float crossTop     = y0 + height + offset * lean;
        float nearShift    = 0.5f * lean;
        float farShift     = (height - 0.5f) * lean;
        for (int side = -1; side <= 1; side += 2)
        {
            float plane = 0.5f + offset * (float) side;
            float low   = plane + nearShift * (float) side;
            float high  = plane - farShift * (float) side;
            // EmitFace4 already emits both windings, so each plane is sent once
            ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layer, rawLight, tint, x0, bottom, z0 + low, x0, crossTop, z0 + high,
                                              x1, crossTop, z0 + high, x1, bottom, z0 + low);
            ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layer, rawLight, tint, x0 + low, bottom, z1, x0 + high, crossTop, z1,
                                              x0 + high, crossTop, z0, x0 + low, bottom, z0);
        }
    }

    float nearX = x0 + sideGap;
    float farX  = x1 - sideGap;
    float nearZ = z0 + sideGap;
    float farZ  = z1 - sideGap;
    if (floor || IsFireBurnable(buildData, x - 1, y, z))
    {
        ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layer, rawLight, tint, nearX, y0, z0, nearX, top, z0, nearX, top, z1,
                                          nearX, y0, z1);
    }

    if (floor || IsFireBurnable(buildData, x + 1, y, z))
    {
        ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layer, rawLight, tint, farX, y0, z1, farX, top, z1, farX, top, z0, farX,
                                          y0, z0);
    }

    if (floor || IsFireBurnable(buildData, x, y, z - 1))
    {
        ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layer, rawLight, tint, x1, y0, nearZ, x1, top, nearZ, x0, top, nearZ, x0,
                                          y0, nearZ);
    }

    if (floor || IsFireBurnable(buildData, x, y, z + 1))
    {
        ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layer, rawLight, tint, x0, y0, farZ, x0, top, farZ, x1, top, farZ, x1, y0,
                                          farZ);
    }

    if (!floor && IsFireBurnable(buildData, x, y + 1, z))
    {
        float high = y0 + 1.0f - sideGap;
        float low  = high - 0.41421356f;
        if ((hash & 1) == 0)
        {
            ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layers[0], rawLight, tint, x1, high, z0, x0, low, z0, x0, low, z1, x1,
                                              high, z1);
            ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layers[1], rawLight, tint, x0, high, z1, x1, low, z1, x1, low, z0, x0,
                                              high, z0);
        }
        else
        {
            ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layers[0], rawLight, tint, x0, high, z1, x0, low, z0, x1, low, z0, x1,
                                              high, z1);
            ChunkMesherFaceEmitter::EmitFace4(context, Direction::up, texture, layers[1], rawLight, tint, x1, high, z0, x1, low, z1, x0, low, z1, x0,
                                              high, z0);
        }
    }
}

void ChunkMesherBuilder::EmitBoxesBlock(FaceEmitContext *context, const Block *block, uint16_t data, int x, int y, int z)
{
    static Direction *directions[] = {Direction::north, Direction::south, Direction::east, Direction::west, Direction::up, Direction::down};

    BuildData &buildData = *context->buildData;
    thread_local std::vector<AABB> boxes;
    thread_local std::vector<AABB> neighborBoxes;
    boxes.clear();
    block->GetRenderBoxes(data, &boxes);

    // Crop boxes sink into farmland, so crops on full blocks are raised to keep their base visible
    if ((dynamic_cast<const CropBlock *>(block) || dynamic_cast<const AttachedStemBlock *>(block)) &&
        ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y - 1, z)) != Block::ByName("farmland"))
    {
        for (size_t i = 0; i < boxes.size(); i++)
        {
            boxes[i] = boxes[i].Translated(glm::dvec3(0.0, 1.0 / 16.0, 0.0));
        }
    }

    for (size_t boxIndex = 0; boxIndex < boxes.size(); boxIndex++)
    {
        const glm::dvec3 &min = boxes[boxIndex].GetMin();
        const glm::dvec3 &max = boxes[boxIndex].GetMax();
        int flatAxis          = -1;
        for (int axis = 0; axis < 3; axis++)
        {
            if (Component(max, axis) - Component(min, axis) <= s_flatBoxThickness)
            {
                flatAxis = axis;
            }
        }

        for (int d = 0; d < 6; d++)
        {
            Direction *direction = directions[d];
            int axis             = direction->dx != 0 ? 0 : (direction->dy != 0 ? 1 : 2);
            // Flat boxes like ladders and vines are planes, their edges would show as stray pixel strips
            if ((flatAxis >= 0 && axis != flatAxis) || !block->HasBoxFace(data, boxIndex, direction))
            {
                continue;
            }

            double plane  = direction->dx + direction->dy + direction->dz > 0 ? Component(max, axis) : Component(min, axis);
            bool boundary = plane == 0.0 || plane == 1.0;
            if (boundary)
            {
                Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x + direction->dx, y + direction->dy, z + direction->dz));
                if (neighbor && neighbor->GetRenderShape() == Block::eRenderShape_CUBE && neighbor->IsSolid() && !neighbor->IsTransparent())
                {
                    continue;
                }

                if (neighbor == block)
                {
                    BlockPos neighborPos(x + direction->dx, y + direction->dy, z + direction->dz);
                    neighborBoxes.clear();
                    block->GetRenderBoxes(ChunkMesherLighting::GetBlockDataLevel(buildData, neighborPos), &neighborBoxes);
                    if (IsFaceCoveredByNeighbor(boxes[boxIndex], neighborBoxes, direction))
                    {
                        continue;
                    }
                }
            }
            else if (IsBoxFaceCovered(boxes, boxIndex, direction))
            {
                continue;
            }

            Texture *texture = block->GetTexture(direction);
            if (!texture)
            {
                continue;
            }

            float x1 = (float) (x + min.x);
            float y1 = (float) (y + min.y);
            float z1 = (float) (z + min.z);
            float x2 = (float) (x + max.x);
            float y2 = (float) (y + max.y);
            float z2 = (float) (z + max.z);

            float corners[4][3];
            float u0;
            float u1;
            float v0 = (float) (1.0 - max.y);
            float v1 = (float) (1.0 - min.y);
            if (direction == Direction::north)
            {
                float quad[4][3] = {{x1, y1, z1}, {x1, y2, z1}, {x2, y2, z1}, {x2, y1, z1}};
                std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
                u0 = (float) (1.0 - max.x);
                u1 = (float) (1.0 - min.x);
            }
            else if (direction == Direction::south)
            {
                float quad[4][3] = {{x2, y1, z2}, {x2, y2, z2}, {x1, y2, z2}, {x1, y1, z2}};
                std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
                u0 = (float) min.x;
                u1 = (float) max.x;
            }
            else if (direction == Direction::east)
            {
                float quad[4][3] = {{x2, y1, z1}, {x2, y2, z1}, {x2, y2, z2}, {x2, y1, z2}};
                std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
                u0 = (float) (1.0 - max.z);
                u1 = (float) (1.0 - min.z);
            }
            else if (direction == Direction::west)
            {
                float quad[4][3] = {{x1, y1, z2}, {x1, y2, z2}, {x1, y2, z1}, {x1, y1, z1}};
                std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
                u0 = (float) min.z;
                u1 = (float) max.z;
            }
            else if (direction == Direction::up)
            {
                float quad[4][3] = {{x1, y2, z1}, {x1, y2, z2}, {x2, y2, z2}, {x2, y2, z1}};
                std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
                u0 = (float) (1.0 - max.x);
                u1 = (float) (1.0 - min.x);
                v0 = (float) (1.0 - max.z);
                v1 = (float) (1.0 - min.z);
            }
            else
            {
                float quad[4][3] = {{x1, y1, z2}, {x1, y1, z1}, {x2, y1, z1}, {x2, y1, z2}};
                std::copy(&quad[0][0], &quad[0][0] + 12, &corners[0][0]);
                u0 = (float) (1.0 - max.x);
                u1 = (float) (1.0 - min.x);
                v0 = (float) min.z;
                v1 = (float) max.z;
            }

            // Faces a pixel short of the cell edge light from the block in front because their own layer reads a torch beside them as fully lit
            bool lightAhead = boundary || (direction->dx + direction->dy + direction->dz > 0 ? 1.0 - plane : plane) <= 1.0 / 16.0;
            int lightX      = x + (lightAhead ? direction->dx : 0);
            int lightY      = y + (lightAhead ? direction->dy : 0);
            int lightZ      = z + (lightAhead ? direction->dz : 0);
            uint16_t rawLight =
                    ChunkMesherCache::SampleLightKey(buildData, context->baseX + lightX, context->baseY + lightY, context->baseZ + lightZ);
            uint32_t tint =
                    block->HasBiomeTint(direction) ? ChunkMesherCache::ResolveTint(buildData, block, direction, x, z, y) : block->GetDataTint(data);

            VertexLight lights[4];
            for (int i = 0; i < 4; i++)
            {
                float corner[3] = {corners[i][0], corners[i][1], corners[i][2]};
                if (!boundary)
                {
                    // Faces inside the cell snap to the cell edge behind them so light and AO come from the block's own layer
                    int normal[3] = {direction->dx, direction->dy, direction->dz};
                    for (int axis = 0; axis < 3; axis++)
                    {
                        if (normal[axis] != 0)
                        {
                            corner[axis] = lightAhead ? (normal[axis] > 0 ? ceilf(corner[axis]) : floorf(corner[axis]))
                                                      : floorf(corner[axis]) + (normal[axis] < 0 ? 1.0f : 0.0f);
                        }
                    }
                }

                lights[i] = block->HasDirectionalShade() ? ChunkMesherCache::BuildVertexLight(buildData, direction,
                                                                                              context->smoothLighting && block->HasAmbientOcclusion(),
                                                                                              rawLight, tint, corner[0], corner[1], corner[2])
                                                         : ChunkMesherCache::BuildVertexLight(buildData, Direction::up, false, rawLight, tint,
                                                                                              corner[0], corner[1], corner[2]);
            }

            bool flipU              = false;
            Block::UVRect atlasRect = block->GetBoxFaceAtlasUVRect(data, boxes[boxIndex], direction, &flipU);
            if (flipU)
            {
                u0 = 1.0f - u0;
                u1 = 1.0f - u1;
            }

            block->AdjustBoxFaceUV(data, boxes[boxIndex], direction, &u0, &v0, &u1, &v1);

            MeshBucket &bucket = context->bucketLookup->Get(texture);
            ChunkMesherFaceEmitter::AddFaceUVSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints, corners[0][0],
                                                    corners[0][1], corners[0][2], corners[1][0], corners[1][1], corners[1][2], corners[2][0],
                                                    corners[2][1], corners[2][2], corners[3][0], corners[3][1], corners[3][2], u0, v0, u1, v1,
                                                    lights[0], lights[1], lights[2], lights[3], tint, atlasRect);

            // Reversed winding with swapped U keeps the texture in place when seen from behind
            if (block->IsBoxFaceDoubleSided(data, boxIndex, direction))
            {
                ChunkMesherFaceEmitter::AddFaceUVSmooth(bucket.vertices, bucket.rawLights, bucket.atlasRects, bucket.shades, bucket.tints,
                                                        corners[3][0], corners[3][1], corners[3][2], corners[2][0], corners[2][1], corners[2][2],
                                                        corners[1][0], corners[1][1], corners[1][2], corners[0][0], corners[0][1], corners[0][2], u1,
                                                        v0, u0, v1, lights[3], lights[2], lights[1], lights[0], tint, atlasRect);
            }
        }
    }

    const DiodeBlock *diode = dynamic_cast<const DiodeBlock *>(block);
    if (diode)
    {
        thread_local std::vector<glm::dvec3> torches;
        thread_local std::vector<bool> torchLit;
        torches.clear();
        torchLit.clear();
        diode->GetTorches(data, &torches, &torchLit);
        uint16_t rawLight = ChunkMesherCache::SampleLightKey(buildData, context->baseX + x, context->baseY + y, context->baseZ + z);
        for (size_t i = 0; i < torches.size(); i++)
        {
            EmitTorchPlanes(context, block->GetTexture(Direction::up), diode->GetTorchAtlasUVRect(torchLit[i]), rawLight,
                            ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z, y), (float) (x + torches[i].x),
                            (float) (y + torches[i].y), (float) (z + torches[i].z), 0.0f, 0.0f);
        }
    }

    thread_local std::vector<Block::RenderPlane> planes;
    planes.clear();
    block->GetRenderPlanes(data, &planes);
    if (planes.empty())
    {
        return;
    }

    Texture *texture        = block->GetTexture(Direction::north);
    Block::UVRect atlasRect = block->GetAtlasUVRect(Direction::north, data);
    uint16_t rawLight       = ChunkMesherCache::SampleLightKey(buildData, context->baseX + x, context->baseY + y, context->baseZ + z);
    uint32_t tint           = ChunkMesherCache::ResolveTint(buildData, block, Direction::north, x, z, y);
    for (size_t i = 0; i < planes.size(); i++)
    {
        const Block::RenderPlane &plane = planes[i];
        glm::dvec3 back                 = plane.halfWidth * -1.0;
        glm::dvec3 normal               = glm::cross(plane.end - plane.start, plane.halfWidth);
        Direction *face                 = fabs(normal.y) >= std::max(fabs(normal.x), fabs(normal.z)) ? Direction::up : Direction::north;
        glm::dvec3 corners[4]           = {plane.start + plane.halfWidth, plane.end + plane.halfWidth, plane.end + back, plane.start + back};
        float points[4][3];
        for (int c = 0; c < 4; c++)
        {
            points[c][0] = (float) (x + corners[c].x);
            points[c][1] = (float) (y + corners[c].y);
            points[c][2] = (float) (z + corners[c].z);
        }

        ChunkMesherFaceEmitter::EmitFace4FixedUV(context, face, texture, atlasRect, rawLight, tint, points[0][0], points[0][1], points[0][2],
                                                 points[1][0], points[1][1], points[1][2], points[2][0], points[2][1], points[2][2], points[3][0],
                                                 points[3][1], points[3][2], plane.u0, plane.v0, plane.u1, plane.v1);
        ChunkMesherFaceEmitter::EmitFace4FixedUV(context, face, texture, atlasRect, rawLight, tint, points[3][0], points[3][1], points[3][2],
                                                 points[2][0], points[2][1], points[2][2], points[1][0], points[1][1], points[1][2], points[0][0],
                                                 points[0][1], points[0][2], plane.u1, plane.v0, plane.u0, plane.v1);
    }
}

void ChunkMesherBuilder::Build(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting, bool grassSideOverlay,
                               std::vector<MeshBuildResult> *outMeshes)
{
    if (!outMeshes || !level || !chunk)
    {
        return;
    }

    outMeshes->clear();

    if (sectionMinY < 0)
    {
        sectionMinY = 0;
    }

    if (sectionMaxY > Chunk::s_sizeY)
    {
        sectionMaxY = Chunk::s_sizeY;
    }

    if (sectionMinY >= sectionMaxY)
    {
        return;
    }

    std::vector<MeshBuildResult> binaryMeshes;
    bool hasBinaryMeshes =
            ChunkMesherBinaryBuilder::BuildBinaryChunkMeshes(level, chunk, sectionMinY, sectionMaxY, smoothLighting, grassSideOverlay, &binaryMeshes);

    thread_local std::unordered_map<Texture *, MeshBucket> buckets;
    thread_local std::unordered_map<Texture *, uint32_t> bucketUsageEpoch;
    thread_local std::vector<Texture *> usedTextures;
    thread_local uint32_t bucketEpochCounter = 0;

    bucketEpochCounter++;
    if (bucketEpochCounter == 0)
    {
        bucketUsageEpoch.clear();
        bucketEpochCounter = 1;
    }

    uint32_t bucketEpoch = bucketEpochCounter;
    usedTextures.clear();

    BucketLookup bucketLookup{&buckets, &bucketUsageEpoch, &usedTextures, bucketEpoch};

    thread_local BuildData buildData;
    buildData.Reset(level, chunk, smoothLighting, sectionMinY, sectionMaxY);
    ChunkMesherCache::BuildSolidCache(&buildData);
    if (smoothLighting)
    {
        ChunkMesherCache::BuildRawLightCache(&buildData);
    }

    ChunkPos chunkPos      = chunk->GetPos();
    int baseX              = chunkPos.x * Chunk::s_sizeX;
    int baseY              = 0;
    int baseZ              = chunkPos.z * Chunk::s_sizeZ;
    int sectionHeight      = sectionMaxY - sectionMinY;
    Block *grassBlock      = Block::ByName("grass_block");
    Block *waterStillBlock = Block::ByName("water_still");
    Block *waterFlowBlock  = Block::ByName("water_flow");
    std::vector<Block::UVRect> waterAtlasRects;
    waterAtlasRects.reserve(12);
    if (waterStillBlock)
    {
        waterAtlasRects.push_back(waterStillBlock->GetAtlasUVRect(Direction::up));
        waterAtlasRects.push_back(waterStillBlock->GetAtlasUVRect(Direction::down));
        waterAtlasRects.push_back(waterStillBlock->GetAtlasUVRect(Direction::north));
        waterAtlasRects.push_back(waterStillBlock->GetAtlasUVRect(Direction::south));
        waterAtlasRects.push_back(waterStillBlock->GetAtlasUVRect(Direction::east));
        waterAtlasRects.push_back(waterStillBlock->GetAtlasUVRect(Direction::west));
    }

    if (waterFlowBlock)
    {
        waterAtlasRects.push_back(waterFlowBlock->GetAtlasUVRect(Direction::up));
        waterAtlasRects.push_back(waterFlowBlock->GetAtlasUVRect(Direction::down));
        waterAtlasRects.push_back(waterFlowBlock->GetAtlasUVRect(Direction::north));
        waterAtlasRects.push_back(waterFlowBlock->GetAtlasUVRect(Direction::south));
        waterAtlasRects.push_back(waterFlowBlock->GetAtlasUVRect(Direction::east));
        waterAtlasRects.push_back(waterFlowBlock->GetAtlasUVRect(Direction::west));
    }

    static const std::vector<Block::UVRect> s_translucentBlockRects = CollectTranslucentBlockRects();
    waterAtlasRects.insert(waterAtlasRects.end(), s_translucentBlockRects.begin(), s_translucentBlockRects.end());

    FaceEmitContext emitContext;
    emitContext.level           = level;
    emitContext.buildData       = &buildData;
    emitContext.bucketLookup    = &bucketLookup;
    emitContext.waterAtlasRects = &waterAtlasRects;
    emitContext.baseX           = baseX;
    emitContext.baseY           = baseY;
    emitContext.baseZ           = baseZ;
    emitContext.smoothLighting  = smoothLighting;

    ChunkMesherWater::Build(buildData, &emitContext, sectionMinY, sectionMaxY, waterStillBlock, waterFlowBlock);
    Block *lavaStillBlock = Block::ByName("lava_still");
    Block *lavaFlowBlock  = Block::ByName("lava_flow");
    if (lavaStillBlock && lavaFlowBlock)
    {
        ChunkMesherWater::Build(buildData, &emitContext, sectionMinY, sectionMaxY, lavaStillBlock, lavaFlowBlock);
    }

    std::vector<GreedyRect> greedyRects;

    {
        std::vector<MaskCell> mask;
        mask.resize((size_t) Chunk::s_sizeZ * (size_t) sectionHeight);

        for (int x = 0; x <= Chunk::s_sizeX; x++)
        {
            for (int y = sectionMinY; y < sectionMaxY; y++)
            {
                for (int z = 0; z < Chunk::s_sizeZ; z++)
                {
                    MaskCell &cell      = mask[(size_t) z + (size_t) Chunk::s_sizeZ * (size_t) (y - sectionMinY)];
                    cell.filled         = false;
                    cell.texture        = nullptr;
                    cell.rawLight       = 0;
                    cell.smoothLight00  = 0;
                    cell.smoothLight01  = 0;
                    cell.smoothLight10  = 0;
                    cell.smoothLight11  = 0;
                    cell.smoothLighting = false;
                    cell.tint           = 0xFFFFFF;
                    cell.shapeFlags     = 0;

                    if (x > 0)
                    {
                        Block *block = Block::ById(OwnedBlockId(buildData, x - 1, y, z));
                        if (block && block->IsLiquid())
                        {
                            continue;
                        }

                        if (hasBinaryMeshes && ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(block))
                        {
                            continue;
                        }

                        Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z));
                        bool shouldEmit = BlockRenderer::TesselateInWorld(block, neighbor, waterStillBlock, waterFlowBlock);
                        shouldEmit      = shouldEmit && !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y, z));
                        shouldEmit      = shouldEmit && !IsCulledLeafFace(buildData, block, neighbor, BlockPos(x + 1, y, z));
                        shouldEmit      = shouldEmit && !IsCoveredByLava(buildData, neighbor, BlockPos(x, y, z), false);
                        if (!shouldEmit)
                        {
                            continue;
                        }

                        Texture *texture = block ? block->GetTexture(Direction::east) : nullptr;
                        if (texture)
                        {
                            cell.filled    = true;
                            cell.texture   = texture;
                            cell.atlasRect = block->GetAtlasUVRect(Direction::east, chunk->GetBlockData(x - 1, y, z));
                            cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + z);
                            cell.tint      = ChunkMesherCache::ResolveTint(buildData, block, Direction::east, x - 1, z, y);

                            if (grassSideOverlay && block == grassBlock)
                            {
                                uint32_t foliageTint = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x - 1, z);
                                cell.tint            = ChunkMesherFaceEmitter::PackTintMode(foliageTint, ChunkMesher::s_tintModeMasked);
                            }

                            if (block == grassBlock)
                            {
                                ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, x - 1, y, z, &cell);
                            }
                        }
                    }
                }
            }

            greedyRects.clear();
            ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeZ, sectionHeight, !smoothLighting, &greedyRects);
            ChunkMesherFaceEmitter::EmitGreedyFaceRects(&emitContext, greedyRects, Direction::east, x, sectionMinY);
        }

        for (int x = 0; x <= Chunk::s_sizeX; x++)
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

                    if (x < Chunk::s_sizeX)
                    {
                        Block *block = Block::ById(OwnedBlockId(buildData, x, y, z));
                        if (block && block->IsLiquid())
                        {
                            continue;
                        }

                        if (hasBinaryMeshes && ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(block))
                        {
                            continue;
                        }

                        Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x - 1, y, z));
                        bool shouldEmit = BlockRenderer::TesselateInWorld(block, neighbor, waterStillBlock, waterFlowBlock);
                        shouldEmit      = shouldEmit && !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x - 1, y, z));
                        shouldEmit      = shouldEmit && !IsCulledLeafFace(buildData, block, neighbor, BlockPos(x - 2, y, z));
                        shouldEmit      = shouldEmit && !IsCoveredByLava(buildData, neighbor, BlockPos(x - 1, y, z), false);
                        if (!shouldEmit)
                        {
                            continue;
                        }

                        Texture *texture = block ? block->GetTexture(Direction::west) : nullptr;
                        if (texture)
                        {
                            cell.filled    = true;
                            cell.texture   = texture;
                            cell.atlasRect = block->GetAtlasUVRect(Direction::west, chunk->GetBlockData(x, y, z));
                            cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x - 1, baseY + y, baseZ + z);
                            cell.tint      = ChunkMesherCache::ResolveTint(buildData, block, Direction::west, x, z, y);

                            if (grassSideOverlay && block == grassBlock)
                            {
                                uint32_t foliageTint = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z);
                                cell.tint            = ChunkMesherFaceEmitter::PackTintMode(foliageTint, ChunkMesher::s_tintModeMasked);
                            }

                            if (block == grassBlock)
                            {
                                ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, x, y, z, &cell);
                            }
                        }
                    }
                }
            }

            greedyRects.clear();
            ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeZ, sectionHeight, !smoothLighting, &greedyRects);
            ChunkMesherFaceEmitter::EmitGreedyFaceRects(&emitContext, greedyRects, Direction::west, x, sectionMinY);
        }
    }

    {
        std::vector<MaskCell> mask;
        mask.resize((size_t) Chunk::s_sizeX * (size_t) sectionHeight);

        for (int z = 0; z <= Chunk::s_sizeZ; z++)
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

                    if (z > 0)
                    {
                        Block *block = Block::ById(OwnedBlockId(buildData, x, y, z - 1));
                        if (block && block->IsLiquid())
                        {
                            continue;
                        }

                        if (hasBinaryMeshes && ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(block))
                        {
                            continue;
                        }

                        Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z));
                        bool shouldEmit = BlockRenderer::TesselateInWorld(block, neighbor, waterStillBlock, waterFlowBlock);
                        shouldEmit      = shouldEmit && !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y, z));
                        shouldEmit      = shouldEmit && !IsCulledLeafFace(buildData, block, neighbor, BlockPos(x, y, z + 1));
                        shouldEmit      = shouldEmit && !IsCoveredByLava(buildData, neighbor, BlockPos(x, y, z), false);
                        if (!shouldEmit)
                        {
                            continue;
                        }

                        Texture *texture = block ? block->GetTexture(Direction::south) : nullptr;
                        if (texture)
                        {
                            cell.filled    = true;
                            cell.texture   = texture;
                            cell.atlasRect = block->GetAtlasUVRect(Direction::south, chunk->GetBlockData(x, y, z - 1));
                            cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + z);
                            cell.tint      = ChunkMesherCache::ResolveTint(buildData, block, Direction::south, x, z - 1, y);

                            if (grassSideOverlay && block == grassBlock)
                            {
                                uint32_t foliageTint = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z - 1);
                                cell.tint            = ChunkMesherFaceEmitter::PackTintMode(foliageTint, ChunkMesher::s_tintModeMasked);
                            }

                            if (block == grassBlock)
                            {
                                ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, x, y, z - 1, &cell);
                            }
                        }
                    }
                }
            }

            greedyRects.clear();
            ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, sectionHeight, !smoothLighting, &greedyRects);
            ChunkMesherFaceEmitter::EmitGreedyFaceRects(&emitContext, greedyRects, Direction::south, z, sectionMinY);
        }

        for (int z = 0; z <= Chunk::s_sizeZ; z++)
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

                    if (z < Chunk::s_sizeZ)
                    {
                        Block *block = Block::ById(OwnedBlockId(buildData, x, y, z));
                        if (block && block->IsLiquid())
                        {
                            continue;
                        }

                        if (hasBinaryMeshes && ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(block))
                        {
                            continue;
                        }

                        Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z - 1));
                        bool shouldEmit = BlockRenderer::TesselateInWorld(block, neighbor, waterStillBlock, waterFlowBlock);
                        shouldEmit      = shouldEmit && !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y, z - 1));
                        shouldEmit      = shouldEmit && !IsCulledLeafFace(buildData, block, neighbor, BlockPos(x, y, z - 2));
                        shouldEmit      = shouldEmit && !IsCoveredByLava(buildData, neighbor, BlockPos(x, y, z - 1), false);
                        if (!shouldEmit)
                        {
                            continue;
                        }

                        Texture *texture = block ? block->GetTexture(Direction::north) : nullptr;
                        if (texture)
                        {
                            cell.filled    = true;
                            cell.texture   = texture;
                            cell.atlasRect = block->GetAtlasUVRect(Direction::north, chunk->GetBlockData(x, y, z));
                            cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + z - 1);
                            cell.tint      = ChunkMesherCache::ResolveTint(buildData, block, Direction::north, x, z, y);

                            if (grassSideOverlay && block == grassBlock)
                            {
                                uint32_t foliageTint = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z);
                                cell.tint            = ChunkMesherFaceEmitter::PackTintMode(foliageTint, ChunkMesher::s_tintModeMasked);
                            }

                            if (block == grassBlock)
                            {
                                ChunkMesherCache::ApplySnowyGrassSide(buildData, grassBlock, x, y, z, &cell);
                            }
                        }
                    }
                }
            }

            greedyRects.clear();
            ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, sectionHeight, !smoothLighting, &greedyRects);
            ChunkMesherFaceEmitter::EmitGreedyFaceRects(&emitContext, greedyRects, Direction::north, z, sectionMinY);
        }

        {
            std::vector<MaskCell> mask;
            mask.resize((size_t) Chunk::s_sizeX * (size_t) Chunk::s_sizeZ);

            for (int y = sectionMinY + 1; y <= sectionMaxY; y++)
            {
                for (int z = 0; z < Chunk::s_sizeZ; z++)
                {
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

                        if (y > 0)
                        {
                            Block *block = Block::ById(OwnedBlockId(buildData, x, y - 1, z));
                            if (block && block->IsLiquid())
                            {
                                continue;
                            }

                            if (hasBinaryMeshes && ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(block))
                            {
                                continue;
                            }

                            Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y, z));
                            bool shouldEmit = BlockRenderer::TesselateInWorld(block, neighbor, waterStillBlock, waterFlowBlock);
                            shouldEmit      = shouldEmit && !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y, z));
                            shouldEmit      = shouldEmit && !IsCulledLeafFace(buildData, block, neighbor, BlockPos(x, y + 1, z));
                            shouldEmit      = shouldEmit && !IsCoveredByLava(buildData, neighbor, BlockPos(x, y, z), true);
                            if (!shouldEmit)
                            {
                                continue;
                            }

                            Texture *texture = block ? block->GetTexture(Direction::up) : nullptr;
                            if (texture)
                            {
                                cell.filled    = true;
                                cell.texture   = texture;
                                cell.atlasRect = block->GetAtlasUVRect(Direction::up, chunk->GetBlockData(x, y - 1, z));
                                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + z);
                                cell.tint      = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z, y);
                            }
                        }
                    }
                }

                greedyRects.clear();
                ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, Chunk::s_sizeZ, !smoothLighting, &greedyRects);
                ChunkMesherFaceEmitter::EmitGreedyFaceRects(&emitContext, greedyRects, Direction::up, y, 0);
            }

            for (int y = sectionMinY; y < sectionMaxY; y++)
            {
                for (int z = 0; z < Chunk::s_sizeZ; z++)
                {
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

                        if (y < Chunk::s_sizeY && y > 0)
                        {
                            Block *block = Block::ById(OwnedBlockId(buildData, x, y, z));
                            if (block && block->IsLiquid())
                            {
                                continue;
                            }

                            if (hasBinaryMeshes && ChunkMesherBinaryBuilder::IsBinaryRenderableBlock(block))
                            {
                                continue;
                            }

                            Block *neighbor = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y - 1, z));
                            bool shouldEmit = BlockRenderer::TesselateInWorld(block, neighbor, waterStillBlock, waterFlowBlock);
                            shouldEmit      = shouldEmit && !ChunkMesherLighting::IsUnloadedLevel(buildData, BlockPos(x, y - 1, z));
                            shouldEmit      = shouldEmit && !IsCulledLeafFace(buildData, block, neighbor, BlockPos(x, y - 2, z));
                            if (!shouldEmit)
                            {
                                continue;
                            }

                            Texture *texture = block ? block->GetTexture(Direction::down) : nullptr;
                            if (texture)
                            {
                                cell.filled    = true;
                                cell.texture   = texture;
                                cell.atlasRect = block->GetAtlasUVRect(Direction::down, chunk->GetBlockData(x, y, z));
                                cell.rawLight  = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y - 1, baseZ + z);
                                cell.tint      = ChunkMesherCache::ResolveTint(buildData, block, Direction::down, x, z, y);
                            }
                        }
                    }
                }

                greedyRects.clear();
                ChunkMesherGreedyMesher::Greedy2D(mask, Chunk::s_sizeX, Chunk::s_sizeZ, !smoothLighting, &greedyRects);
                ChunkMesherFaceEmitter::EmitGreedyFaceRects(&emitContext, greedyRects, Direction::down, y, 0);
            }
        }

        for (int x = 0; x < Chunk::s_sizeX; x++)
        {
            for (int y = sectionMinY; y < sectionMaxY; y++)
            {
                for (int z = 0; z < Chunk::s_sizeZ; z++)
                {
                    Block *block = Block::ById(OwnedBlockId(buildData, x, y, z));
                    if (!block)
                    {
                        continue;
                    }

                    Block::eRENDER_SHAPE renderType = block->GetRenderShape();
                    if (renderType == Block::eRenderShape_FIRE)
                    {
                        EmitFireBlock(&emitContext, block, ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + z),
                                      ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z), x, y, z);
                        continue;
                    }

                    if (renderType == Block::eRenderShape_BOXES)
                    {
                        if (block->IsEntityRendered())
                        {
                            continue;
                        }

                        EmitBoxesBlock(&emitContext, block, chunk->GetBlockData(x, y, z), x, y, z);
                        continue;
                    }

                    if (renderType != Block::eRenderShape_CROSS && renderType != Block::eRenderShape_TORCH)
                    {
                        continue;
                    }

                    Texture *texture = block->GetTexture(Direction::up);
                    if (!texture)
                    {
                        continue;
                    }

                    Block::UVRect atlasUp = block->GetAtlasUVRect(Direction::up);

                    float wx = (float) x;
                    float wy = (float) y;
                    float wz = (float) z;

                    uint16_t rawLight = ChunkMesherCache::SampleLightKey(buildData, baseX + x, baseY + y, baseZ + z);
                    uint32_t tint     = ChunkMesherCache::ResolveTint(buildData, block, Direction::up, x, z);

                    if (renderType == Block::eRenderShape_CROSS)
                    {
                        uint16_t crossData     = chunk->GetBlockData(x, y, z);
                        Direction *attachment  = block->CrossFollowsAttachment() ? block->GetAttachment(crossData) : Direction::down;
                        glm::dvec3 crossOffset = block->GetCrossOffset(
                                BlockPos(chunk->GetPos().x * Chunk::s_sizeX + x, baseY + y, chunk->GetPos().z * Chunk::s_sizeZ + z), crossData);
                        float centerX = 0.5f + (float) crossOffset.x;
                        float centerZ = 0.5f + (float) crossOffset.z;
                        float bottomY = (float) crossOffset.y;
                        if (dynamic_cast<const StemBlock *>(block) &&
                            ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y - 1, z)) != Block::ByName("farmland"))
                        {
                            bottomY += 1.0f / 16.0f;
                        }

                        float topY               = bottomY + 1.0f;
                        Block::UVRect atlasCross = block->GetCrossAtlasUVRect(crossData);

                        float x0 = centerX - 0.45f;
                        float x1 = centerX + 0.45f;
                        float z0 = centerZ - 0.45f;
                        float z1 = centerZ + 0.45f;

                        float localX[8] = {x0, x0, x1, x1, x1, x1, x0, x0};
                        float localY[8] = {bottomY, topY, topY, bottomY, bottomY, topY, topY, bottomY};
                        float localZ[8] = {z0, z0, z1, z1, z0, z0, z1, z1};
                        float px[8];
                        float py[8];
                        float pz[8];
                        for (int i = 0; i < 8; i++)
                        {
                            px[i] = localX[i];
                            py[i] = localY[i];
                            pz[i] = localZ[i];
                            if (attachment->dy > 0)
                            {
                                py[i] = 1.0f - localY[i];
                            }
                            else if (attachment->dz != 0)
                            {
                                py[i] = localZ[i];
                                pz[i] = attachment->dz < 0 ? localY[i] : 1.0f - localY[i];
                            }
                            else if (attachment->dx != 0)
                            {
                                px[i] = attachment->dx < 0 ? localY[i] : 1.0f - localY[i];
                                py[i] = localX[i];
                            }

                            px[i] += wx;
                            py[i] += wy;
                            pz[i] += wz;
                        }

                        ChunkMesherFaceEmitter::EmitFace4(&emitContext, Direction::up, texture, atlasCross, rawLight, tint, px[0], py[0], pz[0],
                                                          px[1], py[1], pz[1], px[2], py[2], pz[2], px[3], py[3], pz[3]);
                        ChunkMesherFaceEmitter::EmitFace4(&emitContext, Direction::up, texture, atlasCross, rawLight, tint, px[4], py[4], pz[4],
                                                          px[5], py[5], pz[5], px[6], py[6], pz[6], px[7], py[7], pz[7]);
                    }
                    else if (block->HasTorchPlanes())
                    {
                        Direction *attachment = block->GetAttachment(chunk->GetBlockData(x, y, z));
                        float lean            = attachment->dy == 0 && block->HasWallMountedTransform() ? 1.0f : 0.0f;
                        EmitTorchPlanes(&emitContext, texture, atlasUp, rawLight, tint, wx + 0.5f + attachment->dx * 0.1f * lean, wy + 0.2f * lean,
                                        wz + 0.5f + attachment->dz * 0.1f * lean, attachment->dx * 0.4f * lean, attachment->dz * 0.4f * lean);
                    }
                    else
                    {
                        const AABB &bb = block->GetAABB();

                        float x1 = wx + bb.GetMin().x;
                        float y1 = wy + bb.GetMin().y;
                        float z1 = wz + bb.GetMin().z;

                        float x2 = wx + bb.GetMax().x;
                        float y2 = wy + bb.GetMax().y;
                        float z2 = wz + bb.GetMax().z;

                        Block::UVRect uvNorth    = block->GetUVRect(Direction::north);
                        Block::UVRect uvSouth    = block->GetUVRect(Direction::south);
                        Block::UVRect uvWest     = block->GetUVRect(Direction::west);
                        Block::UVRect uvEast     = block->GetUVRect(Direction::east);
                        Block::UVRect uvDown     = block->GetUVRect(Direction::down);
                        Block::UVRect uvUp       = block->GetUVRect(Direction::up);
                        Block::UVRect atlasNorth = block->GetAtlasUVRect(Direction::north);
                        Block::UVRect atlasSouth = block->GetAtlasUVRect(Direction::south);
                        Block::UVRect atlasWest  = block->GetAtlasUVRect(Direction::west);
                        Block::UVRect atlasEast  = block->GetAtlasUVRect(Direction::east);
                        Block::UVRect atlasDown  = block->GetAtlasUVRect(Direction::down);

                        Texture *textureUp    = texture;
                        Texture *textureNorth = block->GetTexture(Direction::north);
                        if (!textureNorth)
                        {
                            textureNorth = textureUp;
                        }

                        Texture *textureSouth = block->GetTexture(Direction::south);
                        if (!textureSouth)
                        {
                            textureSouth = textureUp;
                        }

                        Texture *textureWest = block->GetTexture(Direction::west);
                        if (!textureWest)
                        {
                            textureWest = textureUp;
                        }

                        Texture *textureEast = block->GetTexture(Direction::east);
                        if (!textureEast)
                        {
                            textureEast = textureUp;
                        }

                        Texture *textureDown = block->GetTexture(Direction::down);
                        if (!textureDown)
                        {
                            textureDown = textureUp;
                        }

                        Direction *attachmentFace = block->GetAttachment(chunk->GetBlockData(x, y, z));
                        bool wallMounted =
                                block->HasWallMountedTransform() && (attachmentFace == Direction::north || attachmentFace == Direction::south ||
                                                                     attachmentFace == Direction::west || attachmentFace == Direction::east);
                        if (!wallMounted)
                        {
                            if (uvNorth.u0 != uvNorth.u1 && uvNorth.v0 != uvNorth.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFixedUVFace(&emitContext, Direction::north, textureNorth, atlasNorth, rawLight, tint, x1,
                                                                        y1, z1, x2, y2, z1, uvNorth.u0, uvNorth.v0, uvNorth.u1, uvNorth.v1);
                            }

                            if (uvSouth.u0 != uvSouth.u1 && uvSouth.v0 != uvSouth.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFixedUVFace(&emitContext, Direction::south, textureSouth, atlasSouth, rawLight, tint, x1,
                                                                        y1, z2, x2, y2, z2, uvSouth.u0, uvSouth.v0, uvSouth.u1, uvSouth.v1);
                            }

                            if (uvWest.u0 != uvWest.u1 && uvWest.v0 != uvWest.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFixedUVFace(&emitContext, Direction::west, textureWest, atlasWest, rawLight, tint, x1, y1,
                                                                        z1, x1, y2, z2, uvWest.u0, uvWest.v0, uvWest.u1, uvWest.v1);
                            }

                            if (uvEast.u0 != uvEast.u1 && uvEast.v0 != uvEast.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFixedUVFace(&emitContext, Direction::east, textureEast, atlasEast, rawLight, tint, x2, y1,
                                                                        z1, x2, y2, z2, uvEast.u0, uvEast.v0, uvEast.u1, uvEast.v1);
                            }

                            if (uvDown.u0 != uvDown.u1 && uvDown.v0 != uvDown.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFixedUVFace(&emitContext, Direction::down, textureDown, atlasDown, rawLight, tint, x1, y1,
                                                                        z1, x2, y1, z2, uvDown.u0, uvDown.v0, uvDown.u1, uvDown.v1);
                            }

                            if (uvUp.u0 != uvUp.u1 && uvUp.v0 != uvUp.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFixedUVFace(&emitContext, Direction::up, textureUp, atlasUp, rawLight, tint, x1, y2, z1,
                                                                        x2, y2, z2, uvUp.u0, uvUp.v0, uvUp.u1, uvUp.v1);
                            }
                        }
                        else
                        {
                            WallMountedVertex v0{x1, y1, z1};
                            WallMountedVertex v1{x2, y1, z1};
                            WallMountedVertex v2{x2, y2, z1};
                            WallMountedVertex v3{x1, y2, z1};
                            WallMountedVertex v4{x1, y1, z2};
                            WallMountedVertex v5{x2, y1, z2};
                            WallMountedVertex v6{x2, y2, z2};
                            WallMountedVertex v7{x1, y2, z2};

                            WallMountedVertex *vertices[8] = {&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7};

                            float angle = block->GetWallMountedTiltDegrees() * (float) std::numbers::pi / 180.0f;
                            if (attachmentFace == Direction::south || attachmentFace == Direction::west)
                            {
                                angle = -angle;
                            }

                            float pivotX = wx + 0.5f;
                            float pivotY = y1;
                            float pivotZ = wz + 0.5f;

                            float c = cosf(angle);
                            float s = sinf(angle);

                            if (attachmentFace == Direction::north || attachmentFace == Direction::south)
                            {
                                for (int i = 0; i < 8; i++)
                                {
                                    float dy = vertices[i]->y - pivotY;
                                    float dz = vertices[i]->z - pivotZ;
                                    float ny = dy * c - dz * s;
                                    float nz = dy * s + dz * c;

                                    vertices[i]->y = pivotY + ny;
                                    vertices[i]->z = pivotZ + nz;
                                }
                            }
                            else
                            {
                                for (int i = 0; i < 8; i++)
                                {
                                    float dx = vertices[i]->x - pivotX;
                                    float dy = vertices[i]->y - pivotY;
                                    float nx = dx * c - dy * s;
                                    float ny = dx * s + dy * c;

                                    vertices[i]->x = pivotX + nx;
                                    vertices[i]->y = pivotY + ny;
                                }
                            }

                            if (attachmentFace == Direction::north)
                            {
                                for (int i = 0; i < 8; i++)
                                {
                                    vertices[i]->z -= block->GetWallMountedInset();
                                }
                            }
                            else if (attachmentFace == Direction::south)
                            {
                                for (int i = 0; i < 8; i++)
                                {
                                    vertices[i]->z += block->GetWallMountedInset();
                                }
                            }
                            else if (attachmentFace == Direction::west)
                            {
                                for (int i = 0; i < 8; i++)
                                {
                                    vertices[i]->x -= block->GetWallMountedInset();
                                }
                            }
                            else if (attachmentFace == Direction::east)
                            {
                                for (int i = 0; i < 8; i++)
                                {
                                    vertices[i]->x += block->GetWallMountedInset();
                                }
                            }

                            if (uvNorth.u0 != uvNorth.u1 && uvNorth.v0 != uvNorth.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFace4FixedUV(&emitContext, Direction::north, textureNorth, atlasNorth, rawLight, tint,
                                                                         v0.x, v0.y, v0.z, v3.x, v3.y, v3.z, v2.x, v2.y, v2.z, v1.x, v1.y, v1.z,
                                                                         uvNorth.u0, uvNorth.v0, uvNorth.u1, uvNorth.v1);
                            }

                            if (uvSouth.u0 != uvSouth.u1 && uvSouth.v0 != uvSouth.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFace4FixedUV(&emitContext, Direction::south, textureSouth, atlasSouth, rawLight, tint,
                                                                         v5.x, v5.y, v5.z, v6.x, v6.y, v6.z, v7.x, v7.y, v7.z, v4.x, v4.y, v4.z,
                                                                         uvSouth.u0, uvSouth.v0, uvSouth.u1, uvSouth.v1);
                            }

                            if (uvWest.u0 != uvWest.u1 && uvWest.v0 != uvWest.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFace4FixedUV(&emitContext, Direction::west, textureWest, atlasWest, rawLight, tint, v4.x,
                                                                         v4.y, v4.z, v7.x, v7.y, v7.z, v3.x, v3.y, v3.z, v0.x, v0.y, v0.z, uvWest.u0,
                                                                         uvWest.v0, uvWest.u1, uvWest.v1);
                            }

                            if (uvEast.u0 != uvEast.u1 && uvEast.v0 != uvEast.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFace4FixedUV(&emitContext, Direction::east, textureEast, atlasEast, rawLight, tint, v1.x,
                                                                         v1.y, v1.z, v2.x, v2.y, v2.z, v6.x, v6.y, v6.z, v5.x, v5.y, v5.z, uvEast.u0,
                                                                         uvEast.v0, uvEast.u1, uvEast.v1);
                            }

                            if (uvDown.u0 != uvDown.u1 && uvDown.v0 != uvDown.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFace4FixedUV(&emitContext, Direction::down, textureDown, atlasDown, rawLight, tint, v4.x,
                                                                         v4.y, v4.z, v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v5.x, v5.y, v5.z, uvDown.u0,
                                                                         uvDown.v0, uvDown.u1, uvDown.v1);
                            }

                            if (uvUp.u0 != uvUp.u1 && uvUp.v0 != uvUp.v1)
                            {
                                ChunkMesherFaceEmitter::EmitFace4FixedUV(&emitContext, Direction::up, textureUp, atlasUp, rawLight, tint, v3.x, v3.y,
                                                                         v3.z, v7.x, v7.y, v7.z, v6.x, v6.y, v6.z, v2.x, v2.y, v2.z, uvUp.u0, uvUp.v0,
                                                                         uvUp.u1, uvUp.v1);
                            }
                        }
                    }
                }
            }
        }

        ChunkMesherResultBuilder::Build(&buckets, usedTextures, waterAtlasRects, s_translucentBlockRects, binaryMeshes, hasBinaryMeshes, outMeshes);
    }
}

#endif
