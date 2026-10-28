#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesher.h"

#include "ChunkMesherBuilder.h"
#include "ChunkMesherFaceEmitter.h"

bool ChunkMesher::s_ambientOcclusionEnabled = true;
bool ChunkMesher::s_smartLeavesEnabled      = true;
bool ChunkMesher::s_forceFullSkyLight       = false;

uint32_t ChunkMesher::OwnedBlockId(const BuildData &buildData, int x, int y, int z)
{
    if (y < buildData.sectionMinY || y >= buildData.sectionMaxY)
    {
        return 0;
    }

    return buildData.chunk->GetBlockId(x, y, z);
}

void ChunkMesher::BuildSectionMeshes(Level *level, const Chunk *chunk, int sectionIndex, bool smoothLighting, bool grassSideOverlay,
                                     std::vector<MeshBuildResult> *outMeshes)
{
    int sectionMinY = sectionIndex * ChunkSection::s_size;
    int sectionMaxY = sectionMinY + ChunkSection::s_size;
    BuildChunkMeshes(level, chunk, sectionMinY, sectionMaxY, smoothLighting, grassSideOverlay, outMeshes);
}

void ChunkMesher::BuildChunkMeshes(Level *level, const Chunk *chunk, int sectionMinY, int sectionMaxY, bool smoothLighting, bool grassSideOverlay,
                                   std::vector<MeshBuildResult> *outMeshes)
{
    ChunkMesherBuilder::Build(level, chunk, sectionMinY, sectionMaxY, smoothLighting, grassSideOverlay, outMeshes);
}

ChunkMesher::BuildData::BuildData()
{
    level         = nullptr;
    chunk         = nullptr;
    baseX         = 0;
    baseY         = 0;
    baseZ         = 0;
    sectionMinY   = 0;
    sectionMaxY   = 0;
    cacheRawLight = false;
}

void ChunkMesher::BuildData::Reset(Level *newLevel, const Chunk *newChunk, bool shouldCacheRawLight, int newSectionMinY, int newSectionMaxY)
{
    level         = newLevel;
    chunk         = newChunk;
    chunkPos      = chunk->GetPos();
    baseX         = chunkPos.x * Chunk::s_sizeX;
    baseY         = 0;
    baseZ         = chunkPos.z * Chunk::s_sizeZ;
    sectionMinY   = newSectionMinY;
    sectionMaxY   = newSectionMaxY;
    cacheRawLight = shouldCacheRawLight;
    for (int i = 0; i < Chunk::s_sizeX * Chunk::s_sizeZ; i++)
    {
        tintsReady[i] = false;
    }

    for (int dz = -1; dz <= 1; dz++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            if (dx == 0 && dz == 0)
            {
                chunks[dx + 1][dz + 1] = chunk;
            }
            else
            {
                chunks[dx + 1][dz + 1] = level->GetChunk(ChunkPos(chunkPos.x + dx, chunkPos.z + dz));
            }
        }
    }

    solids.resize(s_buildCacheSize);
    if (cacheRawLight)
    {
        rawLights.resize(s_buildCacheSize);
        rawLoaded.resize(s_buildCacheSize);
        smoothLightSums.resize(s_smoothCacheSize);
        smoothLightMeta.resize(s_smoothCacheSize);
        std::fill(rawLights.begin(), rawLights.end(), ChunkMesherFaceEmitter::PackRawLight(0, 0, 0, 0));
        std::fill(rawLoaded.begin(), rawLoaded.end(), (uint8_t) 0);
        std::fill(smoothLightMeta.begin(), smoothLightMeta.end(), (uint8_t) 0);
    }
}

ChunkMesher::MeshBucket &ChunkMesher::BucketLookup::Get(Texture *texture)
{
    auto bucketIt = buckets->find(texture);
    if (bucketIt == buckets->end())
    {
        auto inserted = buckets->insert(std::make_pair(texture, MeshBucket()));
        bucketIt      = inserted.first;
    }

    auto usageIt = usageEpoch->find(texture);
    if (usageIt == usageEpoch->end())
    {
        usageEpoch->insert(std::make_pair(texture, epoch));
        usedTextures->push_back(texture);
        bucketIt->second.vertices.clear();
        bucketIt->second.atlasRects.clear();
        bucketIt->second.rawLights.clear();
        bucketIt->second.shades.clear();
        bucketIt->second.tints.clear();
    }
    else if (usageIt->second != epoch)
    {
        usageIt->second = epoch;
        usedTextures->push_back(texture);
        bucketIt->second.vertices.clear();
        bucketIt->second.atlasRects.clear();
        bucketIt->second.rawLights.clear();
        bucketIt->second.shades.clear();
        bucketIt->second.tints.clear();
    }

    return bucketIt->second;
}

#endif
