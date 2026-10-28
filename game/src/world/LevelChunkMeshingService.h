#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

#include "chunk/ChunkMesh.h"
#include "chunk/ChunkPos.h"
#include "chunk/SectionPos.h"

#include <glm/vec3.hpp>

class LevelRenderer;
class Chunk;

class LevelChunkMeshingService
{
public:
    static void TickMeshing(LevelRenderer *levelRenderer);
    static bool HasChunkMesh(const LevelRenderer *levelRenderer, const ChunkPos &pos);
    static void Rebuild(LevelRenderer *levelRenderer);
    static void RebuildChunk(LevelRenderer *levelRenderer, const ChunkPos &pos);
    static void RebuildChunkUrgent(LevelRenderer *levelRenderer, const ChunkPos &pos);
    static void RebuildChunkDeferred(LevelRenderer *levelRenderer, const ChunkPos &pos, bool urgent);
    static void RebuildSectionUrgent(LevelRenderer *levelRenderer, const SectionPos &pos);
    static void RebuildSectionDeferred(LevelRenderer *levelRenderer, const SectionPos &pos, bool urgent);
    static void RebuildSectionNow(LevelRenderer *levelRenderer, const SectionPos &pos);
    static void FlushScheduling(LevelRenderer *levelRenderer);
    static void RebuildSectionsUrgentSynchronized(LevelRenderer *levelRenderer, const std::vector<SectionPos> &positions);
    static size_t GetPendingMeshCount(const LevelRenderer *levelRenderer);
    static size_t GetQueuedRebuildCount(const LevelRenderer *levelRenderer);
    static size_t GetUrgentRebuildCount(const LevelRenderer *levelRenderer);
    static void UploadPendingMeshes(LevelRenderer *levelRenderer);
    static void PresentReadyMeshes(LevelRenderer *levelRenderer, double deadline);
    static void ScheduleMesher(LevelRenderer *levelRenderer);
    static void CompactRebuildQueues(LevelRenderer *levelRenderer, const glm::dvec3 &cameraPos);
    static void SubmitMesh(LevelRenderer *levelRenderer, const SectionPos &pos, uint64_t generation, uint64_t syncGroup, uint64_t visibility,
                           std::vector<ChunkMesh::PackedData> &&packedResults);
    static void RemoveChunkFromPendingSyncGroups(LevelRenderer *levelRenderer, const SectionPos &pos);
    // The caller has to hold m_rebuildQueueMutex
    static void ForgetSection(LevelRenderer *levelRenderer, const SectionPos &pos);

private:
    static const uint32_t s_meshRadiusShrinkIntervalMs = 1000;
    static const uint32_t s_meshRadiusGrowIntervalMs   = 10000;
    static const int s_minimumMeshRadius               = 4;

    static void UpdateMeshRadius(LevelRenderer *levelRenderer);
    static bool IsInMeshRadius(const LevelRenderer *levelRenderer, const SectionPos &pos, const glm::dvec3 &cameraPos);
    static bool ShouldMeshSection(LevelRenderer *levelRenderer, const Chunk *chunk, const SectionPos &pos);
    static void BuildPackedSection(LevelRenderer *levelRenderer, const Chunk *chunk, const SectionPos &pos, bool smoothLighting,
                                   bool grassSideOverlay, std::vector<ChunkMesh::PackedData> *packed);
    static void EnqueueSection(LevelRenderer *levelRenderer, const SectionPos &pos, bool urgent, uint64_t syncGroup);
    static void ApplyReadyMesh(LevelRenderer *levelRenderer, const SectionPos &pos, std::unordered_set<SectionPos, SectionPosHash> *touchedRegions);
};
