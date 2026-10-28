#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherLighting.h"
#include "ChunkMesherCache.h"
#include "ChunkMesherFaceEmitter.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "../../utils/math/Mth.h"
#include "../LevelSource.h"
#include "../lighting/LightEngine.h"

Direction *ChunkMesherLighting::DecodeDirection(uint8_t id)
{
    if (id == 1)
    {
        return Direction::north;
    }

    if (id == 2)
    {
        return Direction::south;
    }

    if (id == 3)
    {
        return Direction::west;
    }

    if (id == 4)
    {
        return Direction::east;
    }

    if (id == 5)
    {
        return Direction::down;
    }

    if (id == 6)
    {
        return Direction::up;
    }

    return nullptr;
}

uint16_t ChunkMesherLighting::SampleLightKey(Level *level, const Chunk *chunk, int wx, int wy, int wz)
{
    uint16_t rawLight;
    if (SampleLightKeyIfLoaded(level, chunk, wx, wy, wz, &rawLight))
    {
        return rawLight;
    }

    return ChunkMesherFaceEmitter::PackRawLight(0, 0, 0, 0);
}

bool ChunkMesherLighting::SampleLightKeyIfLoaded(Level *level, const Chunk *chunk, int wx, int wy, int wz, uint16_t *outRawLight)
{
    if (!level || !chunk || !outRawLight)
    {
        return false;
    }

    if (wy >= Chunk::s_sizeY)
    {
        *outRawLight = ChunkMesherFaceEmitter::PackRawLight(0, 0, 0, 15);
        return true;
    }

    int localX = wx - chunk->GetPos().x * Chunk::s_sizeX;
    int localY = wy;
    int localZ = wz - chunk->GetPos().z * Chunk::s_sizeZ;

    ChunkPos pos = chunk->GetPos();
    while (localX < 0)
    {
        localX += Chunk::s_sizeX;
        pos.x--;
    }

    while (localX >= Chunk::s_sizeX)
    {
        localX -= Chunk::s_sizeX;
        pos.x++;
    }

    if (localY < 0 || localY >= Chunk::s_sizeY)
    {
        return false;
    }

    while (localZ < 0)
    {
        localZ += Chunk::s_sizeZ;
        pos.z--;
    }

    while (localZ >= Chunk::s_sizeZ)
    {
        localZ -= Chunk::s_sizeZ;
        pos.z++;
    }

    const Chunk *sampleChunk = level->GetChunk(pos);
    if (!sampleChunk)
    {
        return false;
    }

    uint8_t br;
    uint8_t bg;
    uint8_t bb;
    sampleChunk->GetBlockLight(localX, localY, localZ, &br, &bg, &bb);
    *outRawLight = ChunkMesherFaceEmitter::PackRawLight(br, bg, bb, sampleChunk->GetSkyLight(localX, localY, localZ));
    return true;
}

void ChunkMesherLighting::DecodeFaceBasis(Direction *direction, int *nx, int *ny, int *nz, int *ux, int *uy, int *uz, int *vx, int *vy, int *vz)
{
    *nx = direction->dx;
    *ny = direction->dy;
    *nz = direction->dz;

    if (direction == Direction::up || direction == Direction::down)
    {
        *ux = 1;
        *uy = 0;
        *uz = 0;
        *vx = 0;
        *vy = 0;
        *vz = 1;
    }
    else if (direction == Direction::north || direction == Direction::south)
    {
        *ux = 1;
        *uy = 0;
        *uz = 0;
        *vx = 0;
        *vy = 1;
        *vz = 0;
    }
    else
    {
        *ux = 0;
        *uy = 1;
        *uz = 0;
        *vx = 0;
        *vy = 0;
        *vz = 1;
    }
}

void ChunkMesherLighting::ColorFromRawLight(Level *level, uint16_t rawLight, uint32_t tint, float shadeMul, float *r, float *g, float *b)
{
    uint8_t br  = (uint8_t) (rawLight & 15);
    uint8_t bg  = (uint8_t) ((rawLight >> 4) & 15);
    uint8_t bb  = (uint8_t) ((rawLight >> 8) & 15);
    uint8_t sky = (uint8_t) ((rawLight >> 12) & 15);
    if (s_forceFullSkyLight)
    {
        sky = 15;
    }

    uint8_t clamp = level->GetSkyLightClamp();
    if (sky > clamp)
    {
        sky = clamp;
    }

    uint8_t lr = br > sky ? br : sky;
    uint8_t lg = bg > sky ? bg : sky;
    uint8_t lb = bb > sky ? bb : sky;

    float sr;
    float sg;
    float sb;
    ChunkMesherFaceEmitter::Shade(lr, lg, lb, shadeMul, &sr, &sg, &sb);

    *r = sr * (float) ((tint >> 16) & 0xFF) / 255.0f;
    *g = sg * (float) ((tint >> 8) & 0xFF) / 255.0f;
    *b = sb * (float) (tint & 0xFF) / 255.0f;
}

uint16_t ChunkMesherLighting::MaxRawLight(uint16_t a, uint16_t b)
{
    uint8_t abr = (uint8_t) (a & 15);
    uint8_t abg = (uint8_t) ((a >> 4) & 15);
    uint8_t abb = (uint8_t) ((a >> 8) & 15);
    uint8_t ask = (uint8_t) ((a >> 12) & 15);
    uint8_t bbr = (uint8_t) (b & 15);
    uint8_t bbg = (uint8_t) ((b >> 4) & 15);
    uint8_t bbb = (uint8_t) ((b >> 8) & 15);
    uint8_t bsk = (uint8_t) ((b >> 12) & 15);
    return ChunkMesherFaceEmitter::PackRawLight(abr > bbr ? abr : bbr, abg > bbg ? abg : bbg, abb > bbb ? abb : bbb, ask > bsk ? ask : bsk);
}

bool ChunkMesherLighting::SampleRenderRawLightIfLoaded(Level *level, const Chunk *chunk, int wx, int wy, int wz, uint16_t *outRawLight)
{
    if (!SampleLightKeyIfLoaded(level, chunk, wx, wy, wz, outRawLight))
    {
        return false;
    }

    int localX   = wx - chunk->GetPos().x * Chunk::s_sizeX;
    int localY   = wy;
    int localZ   = wz - chunk->GetPos().z * Chunk::s_sizeZ;
    Block *block = GetBlockLevel(level, chunk, BlockPos(localX, localY, localZ));
    if (!block || !block->IsSolid() || block->IsTransparent())
    {
        return true;
    }

    uint16_t neighborRawLight = 0;
    if (SampleLightKeyIfLoaded(level, chunk, wx + 1, wy, wz, &neighborRawLight))
    {
        *outRawLight = MaxRawLight(*outRawLight, neighborRawLight);
    }

    if (SampleLightKeyIfLoaded(level, chunk, wx - 1, wy, wz, &neighborRawLight))
    {
        *outRawLight = MaxRawLight(*outRawLight, neighborRawLight);
    }

    if (SampleLightKeyIfLoaded(level, chunk, wx, wy + 1, wz, &neighborRawLight))
    {
        *outRawLight = MaxRawLight(*outRawLight, neighborRawLight);
    }

    if (SampleLightKeyIfLoaded(level, chunk, wx, wy - 1, wz, &neighborRawLight))
    {
        *outRawLight = MaxRawLight(*outRawLight, neighborRawLight);
    }

    if (SampleLightKeyIfLoaded(level, chunk, wx, wy, wz + 1, &neighborRawLight))
    {
        *outRawLight = MaxRawLight(*outRawLight, neighborRawLight);
    }

    if (SampleLightKeyIfLoaded(level, chunk, wx, wy, wz - 1, &neighborRawLight))
    {
        *outRawLight = MaxRawLight(*outRawLight, neighborRawLight);
    }

    return true;
}

uint32_t ChunkMesherLighting::SampleSmoothRawLight(Level *level, const Chunk *chunk, Direction *direction, uint16_t faceRawLight, float x, float y,
                                                   float z)
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
    DecodeFaceBasis(direction, &nx, &ny, &nz, &ux, &uy, &uz, &vx, &vy, &vz);

    int bx = (int) floorf(x - 0.5f * (float) nx);
    int by = (int) floorf(y - 0.5f * (float) ny);
    int bz = (int) floorf(z - 0.5f * (float) nz);

    float cx = (float) bx + 0.5f;
    float cy = (float) by + 0.5f;
    float cz = (float) bz + 0.5f;

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

    uint16_t s0 = faceRawLight;
    uint16_t s1 = faceRawLight;
    uint16_t s2 = faceRawLight;
    uint16_t s3 = faceRawLight;

    SampleRenderRawLightIfLoaded(level, chunk, bx + nx, by + ny, bz + nz, &s0);
    SampleRenderRawLightIfLoaded(level, chunk, bx + nx + su * ux, by + ny + su * uy, bz + nz + su * uz, &s1);
    SampleRenderRawLightIfLoaded(level, chunk, bx + nx + sv * vx, by + ny + sv * vy, bz + nz + sv * vz, &s2);
    SampleRenderRawLightIfLoaded(level, chunk, bx + nx + su * ux + sv * vx, by + ny + su * uy + sv * vy, bz + nz + su * uz + sv * vz, &s3);

    return ChunkMesherCache::BlendRawLight(s0, s1, s2, s3, faceRawLight);
}

ChunkMesherLighting::VertexLight ChunkMesherLighting::BuildVertexLight(Level *level, const Chunk *chunk, Direction *direction, bool smoothLighting,
                                                                       uint16_t faceRawLight, uint32_t tint, float x, float y, float z)
{
    VertexLight light;
    light.vertexLight = ChunkMesherFaceEmitter::ToVertexLight(faceRawLight);
    if (smoothLighting)
    {
        light.vertexLight = SampleSmoothRawLight(level, chunk, direction, faceRawLight, x, y, z);
    }

    light.shade              = ChunkMesherFaceEmitter::GetDirectionalShade(direction);
    uint32_t tintForLighting = ChunkMesherFaceEmitter::GetTintMode(tint) == s_tintModeMasked ? 0xFFFFFF : tint;
    ColorFromRawLight(level, ChunkMesherFaceEmitter::FromVertexLight(light.vertexLight), tintForLighting, light.shade, &light.r, &light.g, &light.b);
    return light;
}

Block *ChunkMesherLighting::GetBlockLevel(Level *level, const Chunk *chunk, const BlockPos &pos)
{
    LevelSource source(level);
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return nullptr;
    }

    if (pos.x >= 0 && pos.x < Chunk::s_sizeX && pos.z >= 0 && pos.z < Chunk::s_sizeZ)
    {
        return Block::ById(chunk->GetBlockId(pos.x, pos.y, pos.z));
    }

    ChunkPos chunkPos = chunk->GetPos();
    int lx            = pos.x;
    int lz            = pos.z;
    if (lx < 0)
    {
        chunkPos.x--;
        lx += Chunk::s_sizeX;
    }
    else if (lx >= Chunk::s_sizeX)
    {
        chunkPos.x++;
        lx -= Chunk::s_sizeX;
    }

    if (lz < 0)
    {
        chunkPos.z--;
        lz += Chunk::s_sizeZ;
    }
    else if (lz >= Chunk::s_sizeZ)
    {
        chunkPos.z++;
        lz -= Chunk::s_sizeZ;
    }

    const Chunk *neighbor = source.GetChunk(chunkPos);
    if (!neighbor)
    {
        return nullptr;
    }

    return Block::ById(neighbor->GetBlockId(lx, pos.y, lz));
}

Block *ChunkMesherLighting::GetBlockLevel(const BuildData &buildData, const BlockPos &pos)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return nullptr;
    }

    int x              = pos.x;
    int y              = pos.y;
    int z              = pos.z;
    const Chunk *chunk = ChunkMesherCache::GetBuildChunk3D(buildData, &x, &y, &z);
    if (!chunk)
    {
        return nullptr;
    }

    return Block::ById(chunk->GetBlockId(x, y, z));
}

// Faces against a chunk that isn't loaded yet are skipped, the chunk remeshes once its neighbor arrives
bool ChunkMesherLighting::IsUnloadedLevel(const BuildData &buildData, const BlockPos &pos)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return false;
    }

    int x = pos.x;
    int y = pos.y;
    int z = pos.z;
    return ChunkMesherCache::GetBuildChunk3D(buildData, &x, &y, &z) == nullptr;
}

bool ChunkMesherLighting::IsWaterLevel(const BuildData &buildData, const BlockPos &pos, Block *waterStillBlock, Block *waterFlowBlock)
{
    Block *block = GetBlockLevel(buildData, pos);
    return BlockRenderer::IsWaterBlock(block, waterStillBlock, waterFlowBlock);
}

bool ChunkMesherLighting::IsSolidLevel(Level *level, const Chunk *chunk, const BlockPos &pos)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *block = GetBlockLevel(level, chunk, pos);
    if (!block)
    {
        return level->AreEmptyChunksSolidForMeshing();
    }

    return block->IsSolid();
}

bool ChunkMesherLighting::IsTransparentLevel(Level *level, const Chunk *chunk, const BlockPos &pos)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return true;
    }

    Block *block = GetBlockLevel(level, chunk, pos);
    if (!block)
    {
        return true;
    }

    return block->IsTransparent();
}

int ChunkMesherLighting::GetBuildCacheIndex(int x, int y, int z)
{
    return (x + 1) + s_buildCacheX * ((y + 1) + s_buildCacheY * (z + 1));
}

bool ChunkMesherLighting::IsInsideBuildCache(int x, int y, int z)
{
    return x >= -1 && x <= Chunk::s_sizeX && y >= -1 && y <= Chunk::s_sizeY && z >= -1 && z <= Chunk::s_sizeZ;
}

int ChunkMesherLighting::DirectionToSmoothCacheLayer(Direction *direction)
{
    if (direction == Direction::north)
    {
        return 0;
    }

    if (direction == Direction::south)
    {
        return 1;
    }

    if (direction == Direction::west)
    {
        return 2;
    }

    if (direction == Direction::east)
    {
        return 3;
    }

    if (direction == Direction::down)
    {
        return 4;
    }

    if (direction == Direction::up)
    {
        return 5;
    }

    return -1;
}

int ChunkMesherLighting::GetSmoothCacheIndex(int layer, int x, int y, int z)
{
    int vertexIndex = x + s_smoothCacheX * (y + s_smoothCacheY * z);
    return vertexIndex + layer * s_smoothCacheX * s_smoothCacheY * s_smoothCacheZ;
}

uint16_t ChunkMesherLighting::GetBlockDataLevel(const BuildData &buildData, const BlockPos &pos)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return 0;
    }

    int x              = pos.x;
    int y              = pos.y;
    int z              = pos.z;
    const Chunk *chunk = ChunkMesherCache::GetBuildChunk3D(buildData, &x, &y, &z);
    if (!chunk)
    {
        return 0;
    }

    return chunk->GetBlockData(x, y, z);
}

#endif
