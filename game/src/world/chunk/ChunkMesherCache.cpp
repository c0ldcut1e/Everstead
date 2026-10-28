#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherCache.h"
#include "ChunkMesherFaceEmitter.h"
#include "ChunkMesherLighting.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "../../utils/math/Mth.h"
#include "../LevelSource.h"
#include "../biome/Biome.h"
#include "../block/GrassBlock.h"
#include "../block/LeavesBlock.h"
#include "../lighting/LightEngine.h"

bool ChunkMesherCache::IsNearlyIntegral(float value)
{
    return fabsf(value - roundf(value)) <= 0.001f;
}

const Chunk *ChunkMesherCache::GetBuildChunk2D(const BuildData &buildData, int *x, int *z)
{
    int ox = 0;
    int oz = 0;
    if (*x < 0)
    {
        ox = -1;
        *x += Chunk::s_sizeX;
    }
    else if (*x >= Chunk::s_sizeX)
    {
        ox = 1;
        *x -= Chunk::s_sizeX;
    }

    if (*z < 0)
    {
        oz = -1;
        *z += Chunk::s_sizeZ;
    }
    else if (*z >= Chunk::s_sizeZ)
    {
        oz = 1;
        *z -= Chunk::s_sizeZ;
    }

    return buildData.chunks[ox + 1][oz + 1];
}

const Chunk *ChunkMesherCache::GetBuildChunk3D(const BuildData &buildData, int *x, int *y, int *z)
{
    int ox = 0;
    int oz = 0;
    if (*x < 0)
    {
        ox = -1;
        *x += Chunk::s_sizeX;
    }
    else if (*x >= Chunk::s_sizeX)
    {
        ox = 1;
        *x -= Chunk::s_sizeX;
    }

    if (*y < 0 || *y >= Chunk::s_sizeY)
    {
        return nullptr;
    }

    if (*z < 0)
    {
        oz = -1;
        *z += Chunk::s_sizeZ;
    }
    else if (*z >= Chunk::s_sizeZ)
    {
        oz = 1;
        *z -= Chunk::s_sizeZ;
    }

    return buildData.chunks[ox + 1][oz + 1];
}

void ChunkMesherCache::BuildSolidCache(BuildData *buildData)
{
    uint8_t missingSolid = buildData->level->AreEmptyChunksSolidForMeshing() ? 1 : 0;

    int yLo = buildData->sectionMinY - s_sectionMeshMargin;
    if (yLo < -1)
    {
        yLo = -1;
    }

    int yHi = buildData->sectionMaxY + s_sectionMeshMargin;
    if (yHi > Chunk::s_sizeY)
    {
        yHi = Chunk::s_sizeY;
    }

    for (int z = -1; z <= Chunk::s_sizeZ; z++)
    {
        for (int y = yLo; y <= yHi; y++)
        {
            for (int x = -1; x <= Chunk::s_sizeX; x++)
            {
                uint8_t solid = 0;
                if (y >= 0 && y < Chunk::s_sizeY)
                {
                    int lx             = x;
                    int lz             = z;
                    const Chunk *chunk = GetBuildChunk2D(*buildData, &lx, &lz);
                    solid              = missingSolid;
                    if (chunk)
                    {
                        Block *block = Block::ById(chunk->GetBlockId(lx, y, lz));
                        solid        = block && block->IsSolid() ? 1 : 0;
                    }
                }

                buildData->solids[(size_t) ChunkMesherLighting::GetBuildCacheIndex(x, y, z)] = solid;
            }
        }
    }
}

void ChunkMesherCache::BuildRawLightCache(BuildData *buildData)
{
    int yLo = buildData->sectionMinY - s_sectionMeshMargin;
    if (yLo < -1)
    {
        yLo = -1;
    }

    int yHi = buildData->sectionMaxY + s_sectionMeshMargin;
    if (yHi > Chunk::s_sizeY)
    {
        yHi = Chunk::s_sizeY;
    }

    for (int z = -1; z <= Chunk::s_sizeZ; z++)
    {
        for (int y = yLo; y <= yHi; y++)
        {
            for (int x = -1; x <= Chunk::s_sizeX; x++)
            {
                size_t index = (size_t) ChunkMesherLighting::GetBuildCacheIndex(x, y, z);
                if (y >= Chunk::s_sizeY)
                {
                    buildData->rawLights[index] = ChunkMesherFaceEmitter::PackRawLight(0, 0, 0, 15);
                    buildData->rawLoaded[index] = 1;
                    continue;
                }

                int lx             = x;
                int ly             = y;
                int lz             = z;
                const Chunk *chunk = GetBuildChunk3D(*buildData, &lx, &ly, &lz);
                if (!chunk)
                {
                    buildData->rawLights[index] = ChunkMesherFaceEmitter::PackRawLight(0, 0, 0, 0);
                    buildData->rawLoaded[index] = 0;
                    continue;
                }

                uint8_t br;
                uint8_t bg;
                uint8_t bb;
                chunk->GetBlockLight(lx, ly, lz, &br, &bg, &bb);
                buildData->rawLights[index] = ChunkMesherFaceEmitter::PackRawLight(br, bg, bb, chunk->GetSkyLight(lx, ly, lz));
                buildData->rawLoaded[index] = 1;
            }
        }
    }
}

uint16_t ChunkMesherCache::SampleLightKey(const BuildData &buildData, int wx, int wy, int wz)
{
    if (buildData.cacheRawLight)
    {
        int x = wx - buildData.baseX;
        int y = wy - buildData.baseY;
        int z = wz - buildData.baseZ;
        if (ChunkMesherLighting::IsInsideBuildCache(x, y, z))
        {
            return buildData.rawLights[(size_t) ChunkMesherLighting::GetBuildCacheIndex(x, y, z)];
        }
    }

    uint16_t rawLight = 0;
    SampleLightKeyIfLoaded(buildData, wx, wy, wz, &rawLight);
    return rawLight;
}

bool ChunkMesherCache::SampleLightKeyIfLoaded(const BuildData &buildData, int wx, int wy, int wz, uint16_t *outRawLight)
{
    if (buildData.cacheRawLight)
    {
        int x = wx - buildData.baseX;
        int y = wy - buildData.baseY;
        int z = wz - buildData.baseZ;
        if (ChunkMesherLighting::IsInsideBuildCache(x, y, z))
        {
            size_t index = (size_t) ChunkMesherLighting::GetBuildCacheIndex(x, y, z);
            *outRawLight = buildData.rawLights[index];
            return buildData.rawLoaded[index] != 0;
        }
    }

    if (wy - buildData.baseY >= Chunk::s_sizeY)
    {
        *outRawLight = ChunkMesherFaceEmitter::PackRawLight(0, 0, 0, 15);
        return true;
    }

    int x              = wx - buildData.baseX;
    int y              = wy - buildData.baseY;
    int z              = wz - buildData.baseZ;
    const Chunk *chunk = GetBuildChunk3D(buildData, &x, &y, &z);
    if (!chunk)
    {
        return false;
    }

    uint8_t br;
    uint8_t bg;
    uint8_t bb;
    chunk->GetBlockLight(x, y, z, &br, &bg, &bb);
    *outRawLight = ChunkMesherFaceEmitter::PackRawLight(br, bg, bb, chunk->GetSkyLight(x, y, z));
    return true;
}

bool ChunkMesherCache::SampleRenderRawLightIfLoaded(const BuildData &buildData, int wx, int wy, int wz, uint16_t *outRawLight)
{
    if (!SampleLightKeyIfLoaded(buildData, wx, wy, wz, outRawLight))
    {
        return false;
    }

    return !IsLightOpaqueAt(buildData, wx, wy, wz);
}

bool ChunkMesherCache::IsLightOpaqueAt(const BuildData &buildData, int wx, int wy, int wz)
{
    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(wx - buildData.baseX, wy - buildData.baseY, wz - buildData.baseZ));
    return block && block->IsSolid() && !block->IsTransparent();
}

void ChunkMesherCache::AccumulateSmoothRawLightSample(const BuildData &buildData, int wx, int wy, int wz, bool countZeroAsFaceLight,
                                                      SmoothLightAccumulator *accumulator)
{
    uint16_t sample = 0;
    if (!SampleRenderRawLightIfLoaded(buildData, wx, wy, wz, &sample))
    {
        accumulator->missingCount++;
        return;
    }

    if (countZeroAsFaceLight && sample == 0)
    {
        accumulator->missingCount++;
        return;
    }

    accumulator->sumBr = (uint8_t) (accumulator->sumBr + (sample & 15));
    accumulator->sumBg = (uint8_t) (accumulator->sumBg + ((sample >> 4) & 15));
    accumulator->sumBb = (uint8_t) (accumulator->sumBb + ((sample >> 8) & 15));
    accumulator->sumSk = (uint8_t) (accumulator->sumSk + ((sample >> 12) & 15));
}

uint32_t ChunkMesherCache::ResolveSmoothRawLight(uint32_t packedSums, uint8_t faceLightCount, uint16_t faceRawLight)
{
    int sumBr = (int) ChunkMesherFaceEmitter::UnpackSmoothLightSum(packedSums, 0) + (int) (faceRawLight & 15) * (int) faceLightCount;
    int sumBg = (int) ChunkMesherFaceEmitter::UnpackSmoothLightSum(packedSums, 6) + (int) ((faceRawLight >> 4) & 15) * (int) faceLightCount;
    int sumBb = (int) ChunkMesherFaceEmitter::UnpackSmoothLightSum(packedSums, 12) + (int) ((faceRawLight >> 8) & 15) * (int) faceLightCount;
    int sumSk = (int) ChunkMesherFaceEmitter::UnpackSmoothLightSum(packedSums, 18) + (int) ((faceRawLight >> 12) & 15) * (int) faceLightCount;
    return (uint32_t) sumBr | ((uint32_t) sumBg << 8) | ((uint32_t) sumBb << 16) | ((uint32_t) sumSk << 24);
}

uint32_t ChunkMesherCache::SampleSmoothRawLight(const BuildData &buildData, Direction *direction, uint16_t faceRawLight, float x, float y, float z)
{
    int nx;
    int ny;
    int nz;
    int ux;
    int uy;
    int uz;
    int vx;
    int vy;
    int vz;
    ChunkMesherLighting::DecodeFaceBasis(direction, &nx, &ny, &nz, &ux, &uy, &uz, &vx, &vy, &vz);

    int lx = (int) floorf(x - 0.5f * (float) nx);
    int ly = (int) floorf(y - 0.5f * (float) ny);
    int lz = (int) floorf(z - 0.5f * (float) nz);

    float cx = (float) lx + 0.5f;
    float cy = (float) ly + 0.5f;
    float cz = (float) lz + 0.5f;

    int bx = buildData.baseX + lx;
    int by = buildData.baseY + ly;
    int bz = buildData.baseZ + lz;

    float du = 0.0f;
    if (ux != 0)
    {
        du = x - cx;
    }
    else if (uy != 0)
    {
        du = y - cy;
    }
    else
    {
        du = z - cz;
    }

    float dv = 0.0f;
    if (vx != 0)
    {
        dv = x - cx;
    }
    else if (vy != 0)
    {
        dv = y - cy;
    }
    else
    {
        dv = z - cz;
    }

    int su = (du >= 0.0f) ? 1 : -1;
    int sv = (dv >= 0.0f) ? 1 : -1;

    uint16_t s0 = faceRawLight;
    uint16_t s1 = faceRawLight;
    uint16_t s2 = faceRawLight;
    uint16_t s3 = faceRawLight;

    SampleRenderRawLightIfLoaded(buildData, bx + nx, by + ny, bz + nz, &s0);
    SampleRenderRawLightIfLoaded(buildData, bx + nx + su * ux, by + ny + su * uy, bz + nz + su * uz, &s1);
    SampleRenderRawLightIfLoaded(buildData, bx + nx + sv * vx, by + ny + sv * vy, bz + nz + sv * vz, &s2);
    if (!IsLightOpaqueAt(buildData, bx + nx + su * ux, by + ny + su * uy, bz + nz + su * uz) ||
        !IsLightOpaqueAt(buildData, bx + nx + sv * vx, by + ny + sv * vy, bz + nz + sv * vz))
    {
        SampleRenderRawLightIfLoaded(buildData, bx + nx + su * ux + sv * vx, by + ny + su * uy + sv * vy, bz + nz + su * uz + sv * vz, &s3);
    }

    return BlendRawLight(s0, s1, s2, s3, faceRawLight);
}

bool ChunkMesherCache::SampleSmoothRawLightCached(BuildData &buildData, Direction *direction, uint16_t faceRawLight, float x, float y, float z,
                                                  uint32_t *outRawLight)
{
    if (!buildData.cacheRawLight || buildData.smoothLightSums.empty() || buildData.smoothLightMeta.empty())
    {
        return false;
    }

    if (!IsNearlyIntegral(x) || !IsNearlyIntegral(y) || !IsNearlyIntegral(z))
    {
        return false;
    }

    int layer = ChunkMesherLighting::DirectionToSmoothCacheLayer(direction);
    if (layer < 0)
    {
        return false;
    }

    int localX = (int) lroundf(x);
    int localY = (int) lroundf(y);
    int localZ = (int) lroundf(z);

    if (localX < 0 || localX > Chunk::s_sizeX || localY < 0 || localY > Chunk::s_sizeY || localZ < 0 || localZ > Chunk::s_sizeZ)
    {
        return false;
    }

    size_t index = (size_t) ChunkMesherLighting::GetSmoothCacheIndex(layer, localX, localY, localZ);
    uint8_t meta = buildData.smoothLightMeta[index];
    if ((meta & 0x80) == 0)
    {
        int nx;
        int ny;
        int nz;
        int ux;
        int uy;
        int uz;
        int vx;
        int vy;
        int vz;
        ChunkMesherLighting::DecodeFaceBasis(direction, &nx, &ny, &nz, &ux, &uy, &uz, &vx, &vy, &vz);

        int lx = (int) floorf(x - 0.5f * (float) nx);
        int ly = (int) floorf(y - 0.5f * (float) ny);
        int lz = (int) floorf(z - 0.5f * (float) nz);

        float cx = (float) lx + 0.5f;
        float cy = (float) ly + 0.5f;
        float cz = (float) lz + 0.5f;

        int bx = buildData.baseX + lx;
        int by = buildData.baseY + ly;
        int bz = buildData.baseZ + lz;

        float du = 0.0f;
        if (ux != 0)
        {
            du = x - cx;
        }
        else if (uy != 0)
        {
            du = y - cy;
        }
        else
        {
            du = z - cz;
        }

        float dv = 0.0f;
        if (vx != 0)
        {
            dv = x - cx;
        }
        else if (vy != 0)
        {
            dv = y - cy;
        }
        else
        {
            dv = z - cz;
        }

        int su = (du >= 0.0f) ? 1 : -1;
        int sv = (dv >= 0.0f) ? 1 : -1;

        SmoothLightAccumulator accumulator;
        accumulator.sumBr        = 0;
        accumulator.sumBg        = 0;
        accumulator.sumBb        = 0;
        accumulator.sumSk        = 0;
        accumulator.missingCount = 0;

        AccumulateSmoothRawLightSample(buildData, bx + nx, by + ny, bz + nz, true, &accumulator);
        AccumulateSmoothRawLightSample(buildData, bx + nx + su * ux, by + ny + su * uy, bz + nz + su * uz, true, &accumulator);
        AccumulateSmoothRawLightSample(buildData, bx + nx + sv * vx, by + ny + sv * vy, bz + nz + sv * vz, true, &accumulator);
        if (!IsLightOpaqueAt(buildData, bx + nx + su * ux, by + ny + su * uy, bz + nz + su * uz) ||
            !IsLightOpaqueAt(buildData, bx + nx + sv * vx, by + ny + sv * vy, bz + nz + sv * vz))
        {
            AccumulateSmoothRawLightSample(buildData, bx + nx + su * ux + sv * vx, by + ny + su * uy + sv * vy, bz + nz + su * uz + sv * vz, true,
                                           &accumulator);
        }
        else
        {
            accumulator.missingCount++;
        }

        buildData.smoothLightSums[index] =
                ChunkMesherFaceEmitter::PackSmoothLightSums(accumulator.sumBr, accumulator.sumBg, accumulator.sumBb, accumulator.sumSk);
        buildData.smoothLightMeta[index] = (uint8_t) (0x80u | (accumulator.missingCount & 0x7u));
        meta                             = buildData.smoothLightMeta[index];
    }

    *outRawLight = ResolveSmoothRawLight(buildData.smoothLightSums[index], (uint8_t) (meta & 0x7u), faceRawLight);
    return true;
}

uint32_t ChunkMesherCache::BlendRawLight(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t faceRawLight)
{
    uint16_t samples[4] = {a, b, c, d};
    uint32_t sums[4]    = {0, 0, 0, 0};
    for (int i = 0; i < 4; i++)
    {
        uint16_t sample = samples[i] == 0 ? faceRawLight : samples[i];
        for (int channel = 0; channel < 4; channel++)
        {
            sums[channel] += (sample >> (channel * 4)) & 15;
        }
    }

    return sums[0] | (sums[1] << 8) | (sums[2] << 16) | (sums[3] << 24);
}

bool ChunkMesherCache::IsSolidBlockAt(const BuildData &buildData, int wx, int wy, int wz)
{
    BlockPos pos(wx - buildData.baseX, wy - buildData.baseY, wz - buildData.baseZ);
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, pos);
    if (!block)
    {
        return buildData.level->AreEmptyChunksSolidForMeshing();
    }

    return block->IsSolid();
}

bool ChunkMesherCache::IsTranslucentAt(const BuildData &buildData, int wx, int wy, int wz)
{
    if (wy < buildData.baseY || wy >= buildData.baseY + Chunk::s_sizeY)
    {
        return true;
    }

    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(wx - buildData.baseX, wy - buildData.baseY, wz - buildData.baseZ));
    if (!block)
    {
        return true;
    }

    return !block->IsSolid() || block->IsLiquid() || (block->IsTransparent() && block->GetLightOpacity() == 0);
}

float ChunkMesherCache::SampleShadeBrightness(const BuildData &buildData, int wx, int wy, int wz)
{
    if (wy < buildData.baseY || wy >= buildData.baseY + Chunk::s_sizeY)
    {
        return 1.0f;
    }

    Block *block = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(wx - buildData.baseX, wy - buildData.baseY, wz - buildData.baseZ));
    if (!block)
    {
        return 1.0f;
    }

    return (block->IsSolid() && !block->IsLiquid() && (!block->IsTransparent() || block->GetLightOpacity() > 0)) ? 0.2f : 1.0f;
}

float ChunkMesherCache::SampleSmoothAoShade(const BuildData &buildData, Direction *direction, float x, float y, float z)
{
    if (!s_ambientOcclusionEnabled)
    {
        return 1.0f;
    }

    int nx;
    int ny;
    int nz;
    int ux;
    int uy;
    int uz;
    int vx;
    int vy;
    int vz;
    ChunkMesherLighting::DecodeFaceBasis(direction, &nx, &ny, &nz, &ux, &uy, &uz, &vx, &vy, &vz);

    int lx = (int) floorf(x - 0.5f * (float) nx);
    int ly = (int) floorf(y - 0.5f * (float) ny);
    int lz = (int) floorf(z - 0.5f * (float) nz);

    float cx = (float) lx + 0.5f;
    float cy = (float) ly + 0.5f;
    float cz = (float) lz + 0.5f;

    int bx = buildData.baseX + lx;
    int by = buildData.baseY + ly;
    int bz = buildData.baseZ + lz;

    float du = 0.0f;
    if (ux != 0)
    {
        du = x - cx;
    }
    else if (uy != 0)
    {
        du = y - cy;
    }
    else
    {
        du = z - cz;
    }

    float dv = 0.0f;
    if (vx != 0)
    {
        dv = x - cx;
    }
    else if (vy != 0)
    {
        dv = y - cy;
    }
    else
    {
        dv = z - cz;
    }

    int su = du >= 0.0f ? 1 : -1;
    int sv = dv >= 0.0f ? 1 : -1;

    float s0 = SampleShadeBrightness(buildData, bx + nx, by + ny, bz + nz);
    float s1 = SampleShadeBrightness(buildData, bx + nx + su * ux, by + ny + su * uy, bz + nz + su * uz);
    float s2 = SampleShadeBrightness(buildData, bx + nx + sv * vx, by + ny + sv * vy, bz + nz + sv * vz);
    float s3 = s1;
    if (s1 >= 1.0f || s2 >= 1.0f)
    {
        s3 = SampleShadeBrightness(buildData, bx + nx + su * ux + sv * vx, by + ny + su * uy + sv * vy, bz + nz + su * uz + sv * vz);
    }

    float aoShade                  = (s0 + s1 + s2 + s3) * 0.25f;
    float ambientOcclusionStrength = 1.0f;
    ambientOcclusionStrength       = std::clamp(ambientOcclusionStrength, 0.0f, 1.0f);
    return 1.0f + (aoShade - 1.0f) * ambientOcclusionStrength;
}

ChunkMesherCache::VertexLight ChunkMesherCache::BuildVertexLight(BuildData &buildData, Direction *direction, bool smoothLighting,
                                                                 uint16_t faceRawLight, uint32_t tint, float x, float y, float z)
{
    VertexLight light;
    light.vertexLight = ChunkMesherFaceEmitter::ToVertexLight(faceRawLight);
    if (smoothLighting)
    {
        if (!SampleSmoothRawLightCached(buildData, direction, faceRawLight, x, y, z, &light.vertexLight))
        {
            light.vertexLight = SampleSmoothRawLight(buildData, direction, faceRawLight, x, y, z);
        }
    }

    light.shade = ChunkMesherFaceEmitter::GetDirectionalShade(direction);
    if (smoothLighting)
    {
        light.shade *= SampleSmoothAoShade(buildData, direction, x, y, z);
        light.shade = std::clamp(light.shade, 0.0f, 1.0f);
    }

    uint32_t tintForLighting = ChunkMesherFaceEmitter::GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
    ChunkMesherLighting::ColorFromRawLight(buildData.level, ChunkMesherFaceEmitter::FromVertexLight(light.vertexLight), tintForLighting, light.shade,
                                           &light.r, &light.g, &light.b);
    return light;
}

uint8_t ChunkMesherCache::PackSmoothLightByte(float value)
{
    float clamped = std::clamp(value, 0.0f, 1.0f);
    return (uint8_t) (clamped * 255.0f + 0.5f);
}

uint8_t ChunkMesherCache::PackRawLightByte(uint32_t vertexLight, int channel)
{
    uint32_t value = std::min<uint32_t>((vertexLight >> (channel * 8)) & 0xFF, 15 * ChunkMesh::kVertexLightScale);
    return (uint8_t) ((value * 255 + 15 * ChunkMesh::kVertexLightScale / 2) / (15 * ChunkMesh::kVertexLightScale));
}

uint64_t ChunkMesherCache::MakeSmoothLightKey(const VertexLight &light)
{
    uint64_t key = 0;
    key |= (uint64_t) PackRawLightByte(light.vertexLight, 0);
    key |= (uint64_t) PackRawLightByte(light.vertexLight, 1) << 8;
    key |= (uint64_t) PackRawLightByte(light.vertexLight, 2) << 16;
    key |= (uint64_t) PackRawLightByte(light.vertexLight, 3) << 24;
    key |= (uint64_t) PackSmoothLightByte(light.shade) << 32;
    key |= (uint64_t) PackSmoothLightByte(light.r) << 40;
    key |= (uint64_t) PackSmoothLightByte(light.g) << 48;
    key |= (uint64_t) PackSmoothLightByte(light.b) << 56;
    return key;
}

uint8_t ChunkMesherCache::GetSmoothLightKeyByte(uint64_t key, int index)
{
    return (uint8_t) ((key >> (index * 8)) & 0xFF);
}

uint64_t ChunkMesherCache::InterpolateSmoothLightKey(uint64_t key00, uint64_t key10, uint64_t key01, int x, int y, int width, int height)
{
    uint64_t key = 0;
    for (int index = 0; index < 8; index++)
    {
        double value = (double) GetSmoothLightKeyByte(key00, index);
        if (width > 0)
        {
            value += ((double) GetSmoothLightKeyByte(key10, index) - (double) GetSmoothLightKeyByte(key00, index)) * (double) x / (double) width;
        }

        if (height > 0)
        {
            value += ((double) GetSmoothLightKeyByte(key01, index) - (double) GetSmoothLightKeyByte(key00, index)) * (double) y / (double) height;
        }

        int byteValue = (int) std::lround(value);
        byteValue     = std::clamp(byteValue, 0, 255);
        key |= (uint64_t) byteValue << (index * 8);
    }

    return key;
}

void ChunkMesherCache::SetMaskSmoothLights(BuildData &buildData, MaskCell *cell, Direction *direction, bool smoothLighting, float x1, float y1,
                                           float z1, float x2, float y2, float z2)
{
    cell->smoothLight00  = 0;
    cell->smoothLight01  = 0;
    cell->smoothLight10  = 0;
    cell->smoothLight11  = 0;
    cell->smoothLighting = false;
    if (!smoothLighting)
    {
        return;
    }

    VertexLight l1;
    VertexLight l2;
    VertexLight l3;
    VertexLight l4;

    if (direction == Direction::north)
    {
        l1                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y1, z1);
        l2                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y2, z1);
        l3                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y2, z1);
        l4                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y1, z1);
        cell->smoothLight00 = MakeSmoothLightKey(l1);
        cell->smoothLight01 = MakeSmoothLightKey(l2);
        cell->smoothLight11 = MakeSmoothLightKey(l3);
        cell->smoothLight10 = MakeSmoothLightKey(l4);
    }
    else if (direction == Direction::south)
    {
        l1                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y1, z2);
        l2                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y2, z2);
        l3                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y2, z2);
        l4                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y1, z2);
        cell->smoothLight10 = MakeSmoothLightKey(l1);
        cell->smoothLight11 = MakeSmoothLightKey(l2);
        cell->smoothLight01 = MakeSmoothLightKey(l3);
        cell->smoothLight00 = MakeSmoothLightKey(l4);
    }
    else if (direction == Direction::up)
    {
        l1                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y2, z1);
        l2                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y2, z2);
        l3                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y2, z2);
        l4                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y2, z1);
        cell->smoothLight00 = MakeSmoothLightKey(l1);
        cell->smoothLight01 = MakeSmoothLightKey(l2);
        cell->smoothLight11 = MakeSmoothLightKey(l3);
        cell->smoothLight10 = MakeSmoothLightKey(l4);
    }
    else if (direction == Direction::down)
    {
        l1                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y1, z2);
        l2                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y1, z1);
        l3                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y1, z1);
        l4                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y1, z2);
        cell->smoothLight01 = MakeSmoothLightKey(l1);
        cell->smoothLight00 = MakeSmoothLightKey(l2);
        cell->smoothLight10 = MakeSmoothLightKey(l3);
        cell->smoothLight11 = MakeSmoothLightKey(l4);
    }
    else if (direction == Direction::east)
    {
        l1                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y1, z1);
        l2                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y2, z1);
        l3                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y2, z2);
        l4                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x2, y1, z2);
        cell->smoothLight00 = MakeSmoothLightKey(l1);
        cell->smoothLight01 = MakeSmoothLightKey(l2);
        cell->smoothLight11 = MakeSmoothLightKey(l3);
        cell->smoothLight10 = MakeSmoothLightKey(l4);
    }
    else if (direction == Direction::west)
    {
        l1                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y1, z2);
        l2                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y2, z2);
        l3                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y2, z1);
        l4                  = BuildVertexLight(buildData, direction, true, cell->rawLight, cell->tint, x1, y1, z1);
        cell->smoothLight10 = MakeSmoothLightKey(l1);
        cell->smoothLight11 = MakeSmoothLightKey(l2);
        cell->smoothLight01 = MakeSmoothLightKey(l3);
        cell->smoothLight00 = MakeSmoothLightKey(l4);
    }
    else
    {
        return;
    }

    cell->smoothLighting = true;
}

bool ChunkMesherCache::CanMergeMaskCells(const MaskCell &cell, const MaskCell &nextCell)
{
    return nextCell.filled && nextCell.texture == cell.texture && ChunkMesherFaceEmitter::SameUVRect(nextCell.atlasRect, cell.atlasRect) &&
           nextCell.rawLight == cell.rawLight && nextCell.tint == cell.tint && nextCell.shapeFlags == cell.shapeFlags &&
           nextCell.smoothLighting == cell.smoothLighting;
}

bool ChunkMesherCache::CanRepresentSmoothLightRect(const std::vector<MaskCell> &mask, int width, int x, int y, int rectWidth, int rectHeight)
{
    const MaskCell &baseCell = mask[(size_t) x + (size_t) width * (size_t) y];
    if (!baseCell.smoothLighting)
    {
        return true;
    }

    uint64_t key00 = baseCell.smoothLight00;
    uint64_t key10 = mask[(size_t) (x + rectWidth - 1) + (size_t) width * (size_t) y].smoothLight10;
    uint64_t key01 = mask[(size_t) x + (size_t) width * (size_t) (y + rectHeight - 1)].smoothLight01;

    for (int yy = 0; yy < rectHeight; yy++)
    {
        for (int xx = 0; xx < rectWidth; xx++)
        {
            const MaskCell &cell = mask[(size_t) (x + xx) + (size_t) width * (size_t) (y + yy)];
            if (!cell.smoothLighting)
            {
                return false;
            }

            if (cell.smoothLight00 != InterpolateSmoothLightKey(key00, key10, key01, xx, yy, rectWidth, rectHeight))
            {
                return false;
            }

            if (cell.smoothLight01 != InterpolateSmoothLightKey(key00, key10, key01, xx, yy + 1, rectWidth, rectHeight))
            {
                return false;
            }

            if (cell.smoothLight10 != InterpolateSmoothLightKey(key00, key10, key01, xx + 1, yy, rectWidth, rectHeight))
            {
                return false;
            }

            if (cell.smoothLight11 != InterpolateSmoothLightKey(key00, key10, key01, xx + 1, yy + 1, rectWidth, rectHeight))
            {
                return false;
            }
        }
    }

    return true;
}

bool ChunkMesherCache::IsSolidLevel(const BuildData &buildData, const BlockPos &pos)
{
    if (ChunkMesherLighting::IsInsideBuildCache(pos.x, pos.y, pos.z))
    {
        return buildData.solids[(size_t) ChunkMesherLighting::GetBuildCacheIndex(pos.x, pos.y, pos.z)] != 0;
    }

    return ChunkMesherLighting::IsSolidLevel(buildData.level, buildData.chunk, pos);
}

void ChunkMesherCache::BlendColumnTints(const BuildData &buildData, int x, int z)
{
    const int radius = 2;

    uint32_t grass[3]   = {0, 0, 0};
    uint32_t foliage[3] = {0, 0, 0};
    int samples         = 0;
    for (int dz = -radius; dz <= radius; dz++)
    {
        for (int dx = -radius; dx <= radius; dx++)
        {
            int lx             = x + dx;
            int lz             = z + dz;
            const Chunk *chunk = GetBuildChunk2D(buildData, &lx, &lz);
            if (!chunk)
            {
                continue;
            }

            Biome *biome = chunk->GetSurfaceBiome(lx, lz);
            if (!biome)
            {
                continue;
            }

            uint32_t grassColor   = biome->GetGrassColor();
            uint32_t foliageColor = biome->GetFoliageColor();
            for (int channel = 0; channel < 3; channel++)
            {
                grass[channel] += (grassColor >> (16 - channel * 8)) & 0xFF;
                foliage[channel] += (foliageColor >> (16 - channel * 8)) & 0xFF;
            }

            samples++;
        }
    }

    int index                     = x + z * Chunk::s_sizeX;
    buildData.tintsReady[index]   = true;
    buildData.columnBiomes[index] = buildData.chunk->GetSurfaceBiome(x, z);
    buildData.grassTints[index]   = 0xFFFFFF;
    buildData.foliageTints[index] = 0xFFFFFF;
    if (samples == 0)
    {
        return;
    }

    buildData.grassTints[index]   = ((grass[0] / samples) << 16) | ((grass[1] / samples) << 8) | (grass[2] / samples);
    buildData.foliageTints[index] = ((foliage[0] / samples) << 16) | ((foliage[1] / samples) << 8) | (foliage[2] / samples);
}

void ChunkMesherCache::ApplySnowyGrassSide(const BuildData &buildData, const Block *grassBlock, int x, int y, int z, MaskCell *cell)
{
    static const Block *s_snowLayer = Block::ByName("snow");
    static const Block *s_snowBlock = Block::ByName("snow_block");
    Block *above                    = ChunkMesherLighting::GetBlockLevel(buildData, BlockPos(x, y + 1, z));
    if (above != s_snowLayer && above != s_snowBlock)
    {
        return;
    }

    cell->atlasRect = ((const GrassBlock *) grassBlock)->GetSnowySideAtlasUVRect();
    cell->tint      = 0xFFFFFF;
}

uint32_t ChunkMesherCache::ResolveTint(const BuildData &buildData, const Block *block, Direction *direction, int x, int z, int y)
{
    if (!block || !block->HasBiomeTint(direction))
    {
        return 0xFFFFFF;
    }

    if (x < 0 || z < 0 || x >= Chunk::s_sizeX || z >= Chunk::s_sizeZ)
    {
        return block->ResolveTint(direction, buildData.level, buildData.chunk, x, z);
    }

    int index = x + z * Chunk::s_sizeX;
    if (!buildData.tintsReady[index])
    {
        BlendColumnTints(buildData, x, z);
    }

    uint32_t tint = block->ResolveTint(direction, buildData.grassTints[index], buildData.foliageTints[index]);
    Biome *biome  = buildData.columnBiomes[index];
    if (y >= 0 && biome && biome->IsSnowyAt(buildData.baseY + y) && LeavesBlock::IsLeaves(block))
    {
        tint = ChunkMesherFaceEmitter::PackTintMode(tint, ChunkMesher::s_tintModeSnowyLeaf);
    }

    return tint;
}

#endif
