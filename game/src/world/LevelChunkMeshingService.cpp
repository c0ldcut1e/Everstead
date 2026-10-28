#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LevelChunkMeshingService.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>

#include "../core/Everstead.h"
#include "../core/FrameProfiler.h"
#include "../core/Logger.h"
#include "../core/MemoryMonitor.h"
#include "../scene/Camera.h"
#include "../threading/ThreadStorage.h"
#include "../utils/Time.h"
#include "../utils/math/Mth.h"
#include "LevelRenderer.h"
#include "chunk/ChunkMesher.h"
#include "chunk/SectionVisibility.h"

#include <SDL2/SDL.h>

void LevelChunkMeshingService::TickMeshing(LevelRenderer *levelRenderer)
{
    UpdateMeshRadius(levelRenderer);

    {
        FrameProfilerScope scope("UploadPendingMeshes");
        UploadPendingMeshes(levelRenderer);
    }

    {
        FrameProfilerScope scope("ScheduleMesher");
        ScheduleMesher(levelRenderer);
    }
}

void LevelChunkMeshingService::UpdateMeshRadius(LevelRenderer *levelRenderer)
{
    MemoryMonitor *memoryMonitor = MemoryMonitor::GetInstance();
    uint32_t now                 = Time::GetMilliseconds();
    int renderDistance           = levelRenderer->m_level->GetRenderDistance();
    if (now - levelRenderer->m_meshRadiusChangeMs < s_meshRadiusShrinkIntervalMs)
    {
        return;
    }

    levelRenderer->m_meshRadiusChangeMs = now;
    if (memoryMonitor->IsUnderPressure())
    {
        // Pausing meshing would leave holes in the world, so we give up the farthest ring of meshes instead
        int current = levelRenderer->m_meshRadius < 0 ? renderDistance : levelRenderer->m_meshRadius;
        if (current > s_minimumMeshRadius)
        {
            levelRenderer->m_meshRadius       = current - 1;
            levelRenderer->m_meshRadiusGrowMs = now;
            Logger::LogWarn("Memory is low, chunk meshes limited to %d chunks", levelRenderer->m_meshRadius);

            const glm::dvec3 &cameraPos                      = Everstead::GetInstance()->GetCamera()->GetPosition();
            std::vector<std::pair<ChunkPos, Chunk *>> chunks = levelRenderer->m_level->GetLoadedChunks();
            int centerX                                      = Mth::FloorDiv((int) floor(cameraPos.x), Chunk::s_sizeX);
            int centerZ                                      = Mth::FloorDiv((int) floor(cameraPos.z), Chunk::s_sizeZ);
            for (size_t i = 0; i < chunks.size(); i++)
            {
                const ChunkPos &pos = chunks[i].first;
                if (std::max(abs(pos.x - centerX), abs(pos.z - centerZ)) > levelRenderer->m_meshRadius)
                {
                    levelRenderer->DropChunkMeshes(pos);
                }
            }
        }
    }
    else if (levelRenderer->m_meshRadius >= 0 && now - levelRenderer->m_meshRadiusGrowMs >= s_meshRadiusGrowIntervalMs &&
             memoryMonitor->HasHeadroom())
    {
        levelRenderer->m_meshRadiusGrowMs = now;
        levelRenderer->m_meshRadius++;
        if (levelRenderer->m_meshRadius >= renderDistance)
        {
            levelRenderer->m_meshRadius = -1;
        }
    }

    // Dropped or skipped chunks are not queued again on their own, so we re-request the ones inside the radius that have no mesh
    const glm::dvec3 &cameraPos                      = Everstead::GetInstance()->GetCamera()->GetPosition();
    std::vector<std::pair<ChunkPos, Chunk *>> chunks = levelRenderer->m_level->GetLoadedChunks();
    for (size_t i = 0; i < chunks.size(); i++)
    {
        const ChunkPos &pos = chunks[i].first;
        if (!IsInMeshRadius(levelRenderer, SectionPos(pos.x, 0, pos.z), cameraPos) || HasChunkMesh(levelRenderer, pos))
        {
            continue;
        }

        bool requested = false;
        {
            std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
            for (int section = 0; section < Chunk::s_sectionCount && !requested; section++)
            {
                requested = levelRenderer->m_requestedMeshGenerations.count(SectionPos(pos.x, section, pos.z)) != 0;
            }
        }

        if (!requested)
        {
            RebuildChunk(levelRenderer, pos);
        }
    }
}

bool LevelChunkMeshingService::IsInMeshRadius(const LevelRenderer *levelRenderer, const SectionPos &pos, const glm::dvec3 &cameraPos)
{
    if (levelRenderer->m_meshRadius < 0)
    {
        return true;
    }

    int centerX = Mth::FloorDiv((int) floor(cameraPos.x), Chunk::s_sizeX);
    int centerZ = Mth::FloorDiv((int) floor(cameraPos.z), Chunk::s_sizeZ);
    return std::max(abs(pos.x - centerX), abs(pos.z - centerZ)) <= levelRenderer->m_meshRadius;
}

bool LevelChunkMeshingService::HasChunkMesh(const LevelRenderer *levelRenderer, const ChunkPos &pos)
{
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        SectionPos sectionPos(pos.x, section, pos.z);
        if (levelRenderer->HasTranslucentSection(sectionPos) || levelRenderer->HasRegionSection(sectionPos))
        {
            return true;
        }
    }

    return false;
}

bool LevelChunkMeshingService::ShouldMeshSection(LevelRenderer *levelRenderer, const Chunk *chunk, const SectionPos &pos)
{
    if (pos.y < 0 || pos.y >= Chunk::s_sectionCount)
    {
        return false;
    }

    if (!chunk->IsSectionEmpty(pos.y))
    {
        return true;
    }

    return levelRenderer->HasTranslucentSection(pos) || levelRenderer->HasRegionSection(pos);
}

void LevelChunkMeshingService::EnqueueSection(LevelRenderer *levelRenderer, const SectionPos &pos, bool urgent, uint64_t syncGroup)
{
    // A light update remeshes edited sections again right away, so we keep them in their edit's group instead of letting one side swap early
    auto syncIt = levelRenderer->m_requestedMeshSyncGroups.find(pos);
    if (syncGroup == 0 && syncIt != levelRenderer->m_requestedMeshSyncGroups.end())
    {
        syncGroup = syncIt->second;
    }

    RemoveChunkFromPendingSyncGroups(levelRenderer, pos);
    levelRenderer->m_requestedMeshGenerations[pos] = ++levelRenderer->m_nextMeshGeneration;
    levelRenderer->m_requestedMeshSyncGroups[pos]  = syncGroup;
    if (syncGroup != 0)
    {
        levelRenderer->m_pendingSyncGroups[syncGroup].insert(pos);
    }

    if (levelRenderer->m_activeRebuilds.find(pos) != levelRenderer->m_activeRebuilds.end())
    {
        if (urgent)
        {
            levelRenderer->m_deferredRebuilds.erase(pos);
            levelRenderer->m_deferredUrgentRebuilds.insert(pos);
        }
        else if (levelRenderer->m_deferredUrgentRebuilds.find(pos) == levelRenderer->m_deferredUrgentRebuilds.end())
        {
            levelRenderer->m_deferredRebuilds.insert(pos);
        }

        return;
    }

    if (urgent)
    {
        if (levelRenderer->m_urgentQueued.insert(pos).second)
        {
            levelRenderer->m_urgentQueue.push_back(pos);
        }
    }
    else
    {
        if (levelRenderer->m_rebuildQueued.insert(pos).second)
        {
            levelRenderer->m_rebuildQueue.push_back(pos);
        }
    }
}

void LevelChunkMeshingService::Rebuild(LevelRenderer *levelRenderer)
{
    levelRenderer->m_translucentRegions.clear();
    levelRenderer->m_renderRegions.clear();
    levelRenderer->ClearRegionDraws();
    levelRenderer->m_columnVisibility.clear();
    levelRenderer->m_meshedSections.clear();
    levelRenderer->m_meshedSectionsVersion++;
    levelRenderer->m_readyMeshes.clear();
    {
        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
        levelRenderer->m_requestedMeshGenerations.clear();
        levelRenderer->m_requestedMeshSyncGroups.clear();
        levelRenderer->m_pendingSyncGroups.clear();
    }

    bool smoothLighting   = Everstead::GetInstance()->options.smoothLighting;
    bool grassSideOverlay = levelRenderer->m_grassSideOverlayEnabled;

    std::vector<std::pair<ChunkPos, Chunk *>> chunks = levelRenderer->m_level->GetLoadedChunks();
    for (auto chunkIt = chunks.begin(); chunkIt != chunks.end(); ++chunkIt)
    {
        const ChunkPos &pos = chunkIt->first;
        Chunk *chunkPtr     = chunkIt->second;

        for (int section = 0; section < Chunk::s_sectionCount; section++)
        {
            if (chunkPtr->IsSectionEmpty(section))
            {
                continue;
            }

            SectionPos sectionPos(pos.x, section, pos.z);
            levelRenderer->SetSectionVisibility(sectionPos, SectionVisibility::Compute(chunkPtr, section));
            std::vector<ChunkMesher::MeshBuildResult> results;
            ChunkMesher::BuildSectionMeshes(levelRenderer->m_level, chunkPtr, section, smoothLighting, grassSideOverlay, &results);

            std::vector<ChunkMesh::PackedData> translucentMeshes;
            std::vector<ChunkMesh::PackedData> opaqueMeshes;
            opaqueMeshes.reserve(results.size());

            for (ChunkMesher::MeshBuildResult &result : results)
            {
                ChunkMesh::PackedData packed = ChunkMesh::PackData(
                        result.vertices.data(), (uint32_t) result.vertices.size(), std::move(result.atlasRects), std::move(result.rawLights),
                        std::move(result.shades), std::move(result.tints), result.texture, result.translucent);
                packed.sortQuads = result.sortQuads;
                if (packed.translucent)
                {
                    translucentMeshes.push_back(std::move(packed));
                }
                else
                {
                    opaqueMeshes.push_back(std::move(packed));
                }
            }

            if (!opaqueMeshes.empty())
            {
                levelRenderer->SetRegionSection(sectionPos, std::move(opaqueMeshes), false);
            }

            if (!translucentMeshes.empty())
            {
                levelRenderer->SetTranslucentSection(sectionPos, std::move(translucentMeshes));
            }
        }
    }

    for (auto &region : levelRenderer->m_renderRegions)
    {
        region.second->Rebuild();
    }
}

void LevelChunkMeshingService::RebuildChunk(LevelRenderer *levelRenderer, const ChunkPos &pos)
{
    Chunk *chunk = levelRenderer->m_level->GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
        for (int section = 0; section < Chunk::s_sectionCount; section++)
        {
            SectionPos sectionPos(pos.x, section, pos.z);
            if (ShouldMeshSection(levelRenderer, chunk, sectionPos))
            {
                EnqueueSection(levelRenderer, sectionPos, false, 0);
            }
        }
    }

    ScheduleMesher(levelRenderer);
}

void LevelChunkMeshingService::RebuildChunkUrgent(LevelRenderer *levelRenderer, const ChunkPos &pos)
{
    Chunk *chunk = levelRenderer->m_level->GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
        for (int section = 0; section < Chunk::s_sectionCount; section++)
        {
            SectionPos sectionPos(pos.x, section, pos.z);
            if (ShouldMeshSection(levelRenderer, chunk, sectionPos))
            {
                EnqueueSection(levelRenderer, sectionPos, true, 0);
            }
        }
    }

    ScheduleMesher(levelRenderer);
}

void LevelChunkMeshingService::RebuildChunkDeferred(LevelRenderer *levelRenderer, const ChunkPos &pos, bool urgent)
{
    Chunk *chunk = levelRenderer->m_level->GetChunk(pos);
    if (!chunk)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        SectionPos sectionPos(pos.x, section, pos.z);
        if (ShouldMeshSection(levelRenderer, chunk, sectionPos))
        {
            EnqueueSection(levelRenderer, sectionPos, urgent, 0);
        }
    }
}

void LevelChunkMeshingService::RebuildSectionUrgent(LevelRenderer *levelRenderer, const SectionPos &pos)
{
    Chunk *chunk = levelRenderer->m_level->GetChunk(pos.ToChunkPos());
    if (!chunk)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
        if (ShouldMeshSection(levelRenderer, chunk, pos))
        {
            EnqueueSection(levelRenderer, pos, true, 0);
        }
    }

    ScheduleMesher(levelRenderer);
}

void LevelChunkMeshingService::RebuildSectionDeferred(LevelRenderer *levelRenderer, const SectionPos &pos, bool urgent)
{
    Chunk *chunk = levelRenderer->m_level->GetChunk(pos.ToChunkPos());
    if (!chunk)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
    if (ShouldMeshSection(levelRenderer, chunk, pos))
    {
        EnqueueSection(levelRenderer, pos, urgent, 0);
    }
}

void LevelChunkMeshingService::BuildPackedSection(LevelRenderer *levelRenderer, const Chunk *chunk, const SectionPos &pos, bool smoothLighting,
                                                  bool grassSideOverlay, std::vector<ChunkMesh::PackedData> *packed)
{
    std::vector<ChunkMesher::MeshBuildResult> results;
    ChunkMesher::BuildSectionMeshes(levelRenderer->m_level, chunk, pos.y, smoothLighting, grassSideOverlay, &results);

    packed->reserve(results.size());
    for (auto resultIt = results.begin(); resultIt != results.end(); ++resultIt)
    {
        ChunkMesher::MeshBuildResult &result = *resultIt;
        packed->push_back(ChunkMesh::PackData(result.vertices.data(), (uint32_t) result.vertices.size(), std::move(result.atlasRects),
                                              std::move(result.rawLights), std::move(result.shades), std::move(result.tints), result.texture,
                                              result.translucent));
        packed->back().sortQuads = result.sortQuads;
    }
}

void LevelChunkMeshingService::RebuildSectionNow(LevelRenderer *levelRenderer, const SectionPos &pos)
{
    Chunk *chunk = levelRenderer->m_level->GetChunk(pos.ToChunkPos());
    if (!chunk)
    {
        return;
    }

    uint64_t generation = 0;
    {
        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
        if (!ShouldMeshSection(levelRenderer, chunk, pos))
        {
            return;
        }

        RemoveChunkFromPendingSyncGroups(levelRenderer, pos);
        generation                                     = ++levelRenderer->m_nextMeshGeneration;
        levelRenderer->m_requestedMeshGenerations[pos] = generation;
        levelRenderer->m_requestedMeshSyncGroups[pos]  = 0;
    }

    std::vector<ChunkMesh::PackedData> packed;
    BuildPackedSection(levelRenderer, chunk, pos, Everstead::GetInstance()->options.smoothLighting, levelRenderer->m_grassSideOverlayEnabled,
                       &packed);

    std::vector<ChunkMesh::PackedData> translucentMeshes;
    std::vector<ChunkMesh::PackedData> opaqueMeshes;
    for (size_t i = 0; i < packed.size(); i++)
    {
        if (packed[i].translucent)
        {
            translucentMeshes.push_back(std::move(packed[i]));
        }
        else
        {
            opaqueMeshes.push_back(std::move(packed[i]));
        }
    }

    levelRenderer->m_readyMeshes[pos] = {
            generation, false, 0, SectionVisibility::Compute(chunk, pos.y), std::move(translucentMeshes), std::move(opaqueMeshes)};
}

void LevelChunkMeshingService::FlushScheduling(LevelRenderer *levelRenderer)
{
    ScheduleMesher(levelRenderer);
}

void LevelChunkMeshingService::RebuildSectionsUrgentSynchronized(LevelRenderer *levelRenderer, const std::vector<SectionPos> &positions)
{
    std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
    uint64_t syncGroup = Everstead::GetInstance()->options.synchronizedBlockUpdates ? levelRenderer->m_nextSyncGroupId++ : 0;
    for (size_t i = 0; i < positions.size(); i++)
    {
        Chunk *chunk = levelRenderer->m_level->GetChunk(positions[i].ToChunkPos());
        if (chunk && ShouldMeshSection(levelRenderer, chunk, positions[i]))
        {
            EnqueueSection(levelRenderer, positions[i], true, syncGroup);
        }
    }

    if (levelRenderer->m_pendingSyncGroups[syncGroup].empty())
    {
        levelRenderer->m_pendingSyncGroups.erase(syncGroup);
    }
}

size_t LevelChunkMeshingService::GetPendingMeshCount(const LevelRenderer *levelRenderer)
{
    std::lock_guard<std::mutex> lock(levelRenderer->m_meshQueueMutex);
    return levelRenderer->m_pendingMeshes.size();
}

size_t LevelChunkMeshingService::GetQueuedRebuildCount(const LevelRenderer *levelRenderer)
{
    std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
    return levelRenderer->m_rebuildQueued.size();
}

size_t LevelChunkMeshingService::GetUrgentRebuildCount(const LevelRenderer *levelRenderer)
{
    std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
    return levelRenderer->m_urgentQueued.size();
}

void LevelChunkMeshingService::UploadPendingMeshes(LevelRenderer *levelRenderer)
{
    double startTime = (double) SDL_GetPerformanceCounter() / (double) SDL_GetPerformanceFrequency();
    // We scale the budget by the time since the last upload so a capped frame rate like vsync uploads as fast as a high one
    double budgetScale              = std::clamp((startTime - levelRenderer->m_lastUploadTime) * 240.0, 1.0, 3.0);
    levelRenderer->m_lastUploadTime = startTime;
    double timeBudget               = 0.0015 * budgetScale;
    int uploadsPerFrame             = (int) (Everstead::GetInstance()->options.chunkUploadsPerFrame * budgetScale);

    // Meshes left over from a frame that ran out of time count against this frame's uploads, so the backlog cannot grow
    for (auto readyIt = levelRenderer->m_readyMeshes.begin(); readyIt != levelRenderer->m_readyMeshes.end(); ++readyIt)
    {
        if (readyIt->second.syncGroup == 0)
        {
            uploadsPerFrame--;
        }
    }

    while (uploadsPerFrame-- > 0)
    {
        LevelRenderer::PendingMeshUpload pendingUpload;

        {
            std::lock_guard<std::mutex> lock(levelRenderer->m_meshQueueMutex);

            if (levelRenderer->m_pendingMeshes.empty())
            {
                break;
            }

            pendingUpload = std::move(levelRenderer->m_pendingMeshes.front());
            levelRenderer->m_pendingMeshes.pop_front();
        }

        const SectionPos &pos = pendingUpload.pos;

        {
            std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
            auto generationIt = levelRenderer->m_requestedMeshGenerations.find(pos);
            if (generationIt == levelRenderer->m_requestedMeshGenerations.end() || pendingUpload.generation != generationIt->second)
            {
                // Superseded results cost nothing to drop, so they must not eat the per-frame upload budget
                uploadsPerFrame++;
                continue;
            }
        }

        std::vector<ChunkMesh::PackedData> &packed = pendingUpload.packedResults;

        std::vector<ChunkMesh::PackedData> translucentMeshes;
        std::vector<ChunkMesh::PackedData> opaqueMeshes;
        opaqueMeshes.reserve(packed.size());

        for (auto packedIt = packed.begin(); packedIt != packed.end(); ++packedIt)
        {
            ChunkMesh::PackedData &data = *packedIt;
            if (data.translucent)
            {
                translucentMeshes.push_back(std::move(data));
            }
            else
            {
                opaqueMeshes.push_back(std::move(data));
            }
        }

        levelRenderer->m_readyMeshes[pos] = {pendingUpload.generation,     false,
                                             pendingUpload.syncGroup,      pendingUpload.visibility,
                                             std::move(translucentMeshes), std::move(opaqueMeshes)};
    }

    PresentReadyMeshes(levelRenderer, startTime + timeBudget);
}

void LevelChunkMeshingService::ApplyReadyMesh(LevelRenderer *levelRenderer, const SectionPos &pos,
                                              std::unordered_set<SectionPos, SectionPosHash> *touchedRegions)
{
    LevelRenderer::ReadyMeshSwap &readyMeshes = levelRenderer->m_readyMeshes[pos];
    levelRenderer->SetSectionVisibility(pos, readyMeshes.visibility);
    std::vector<LevelRenderer::SettlingBlock> &settlingBlocks = levelRenderer->m_settlingBlocks;
    for (size_t i = 0; i < settlingBlocks.size();)
    {
        const LevelRenderer::SettlingBlock &settling = settlingBlocks[i];
        if (readyMeshes.generation >= settling.generation &&
            pos == SectionPos(Mth::FloorDiv(settling.pos.x, Chunk::s_sizeX), settling.pos.y / ChunkSection::s_size,
                              Mth::FloorDiv(settling.pos.z, Chunk::s_sizeZ)))
        {
            settlingBlocks.erase(settlingBlocks.begin() + i);
            continue;
        }

        i++;
    }

    if (readyMeshes.opaqueMeshes.empty())
    {
        levelRenderer->RemoveRegionSection(pos);
    }
    else
    {
        SectionPos regionPos = ChunkRenderRegion::GetRegionPos(pos);
        auto regionIt        = levelRenderer->m_renderRegions.find(regionPos);
        bool updatedInPlace  = regionIt != levelRenderer->m_renderRegions.end() && touchedRegions->find(regionPos) == touchedRegions->end() &&
                               regionIt->second->UpdateSection(pos, readyMeshes.opaqueMeshes);
        if (!updatedInPlace)
        {
            levelRenderer->SetRegionSection(pos, std::move(readyMeshes.opaqueMeshes), false);
            touchedRegions->insert(regionPos);
        }
    }

    if (readyMeshes.translucentMeshes.empty())
    {
        levelRenderer->RemoveTranslucentSection(pos);
        return;
    }

    levelRenderer->SetTranslucentSection(pos, std::move(readyMeshes.translucentMeshes));
}

void LevelChunkMeshingService::PresentReadyMeshes(LevelRenderer *levelRenderer, double deadline)
{
    std::lock_guard<std::mutex> rebuildLock(levelRenderer->m_rebuildQueueMutex);
    std::unordered_set<SectionPos, SectionPosHash> touchedRegions;

    for (auto readyIt = levelRenderer->m_readyMeshes.begin(); readyIt != levelRenderer->m_readyMeshes.end();)
    {
        const SectionPos &pos                = readyIt->first;
        LevelRenderer::ReadyMeshSwap &meshes = readyIt->second;

        auto generationIt            = levelRenderer->m_requestedMeshGenerations.find(pos);
        uint64_t requestedGeneration = generationIt != levelRenderer->m_requestedMeshGenerations.end() ? generationIt->second : 0;

        auto syncIt                 = levelRenderer->m_requestedMeshSyncGroups.find(pos);
        uint64_t requestedSyncGroup = syncIt != levelRenderer->m_requestedMeshSyncGroups.end() ? syncIt->second : 0;

        if (meshes.generation != requestedGeneration || meshes.syncGroup != requestedSyncGroup)
        {
            readyIt = levelRenderer->m_readyMeshes.erase(readyIt);
            continue;
        }

        ++readyIt;
    }

    for (auto groupIt = levelRenderer->m_pendingSyncGroups.begin(); groupIt != levelRenderer->m_pendingSyncGroups.end();)
    {
        uint64_t syncGroup                                              = groupIt->first;
        const std::unordered_set<SectionPos, SectionPosHash> &groupSecs = groupIt->second;

        bool ready = true;
        for (auto groupSecIt = groupSecs.begin(); groupSecIt != groupSecs.end(); ++groupSecIt)
        {
            const SectionPos &pos = *groupSecIt;
            auto readyMeshIt      = levelRenderer->m_readyMeshes.find(pos);
            if (readyMeshIt == levelRenderer->m_readyMeshes.end())
            {
                ready = false;
                break;
            }

            if (readyMeshIt->second.syncGroup != syncGroup)
            {
                ready = false;
                break;
            }

            auto generationIt = levelRenderer->m_requestedMeshGenerations.find(pos);
            if (generationIt == levelRenderer->m_requestedMeshGenerations.end() || readyMeshIt->second.generation != generationIt->second)
            {
                ready = false;
                break;
            }
        }

        if (!ready)
        {
            ++groupIt;
            continue;
        }

        for (auto groupSecIt = groupSecs.begin(); groupSecIt != groupSecs.end(); ++groupSecIt)
        {
            const SectionPos &pos = *groupSecIt;
            ApplyReadyMesh(levelRenderer, pos, &touchedRegions);
            levelRenderer->m_readyMeshes.erase(pos);
            levelRenderer->m_requestedMeshSyncGroups.erase(pos);
        }

        groupIt = levelRenderer->m_pendingSyncGroups.erase(groupIt);
    }

    for (auto readyIt = levelRenderer->m_readyMeshes.begin(); readyIt != levelRenderer->m_readyMeshes.end();)
    {
        const SectionPos &pos                     = readyIt->first;
        LevelRenderer::ReadyMeshSwap &readyMeshes = readyIt->second;

        if (readyMeshes.syncGroup != 0)
        {
            ++readyIt;
            continue;
        }

        ApplyReadyMesh(levelRenderer, pos, &touchedRegions);
        readyIt = levelRenderer->m_readyMeshes.erase(readyIt);
        // Writing vertices to the GPU is the expensive part, so this is where the frame budget applies
        if ((double) SDL_GetPerformanceCounter() / (double) SDL_GetPerformanceFrequency() >= deadline)
        {
            break;
        }
    }

    for (const SectionPos &regionPos : touchedRegions)
    {
        auto regionIt = levelRenderer->m_renderRegions.find(regionPos);
        if (regionIt != levelRenderer->m_renderRegions.end())
        {
            regionIt->second->Rebuild();
            levelRenderer->m_dirtyRegions.erase(regionPos);
        }
    }
}

void LevelChunkMeshingService::CompactRebuildQueues(LevelRenderer *levelRenderer, const glm::dvec3 &cameraPos)
{
    std::vector<SectionPos> *queues[2]                            = {&levelRenderer->m_urgentQueue, &levelRenderer->m_rebuildQueue};
    std::unordered_set<SectionPos, SectionPosHash> *queuedSets[2] = {&levelRenderer->m_urgentQueued, &levelRenderer->m_rebuildQueued};
    for (int q = 0; q < 2; q++)
    {
        std::vector<SectionPos> &queue = *queues[q];
        size_t kept                    = 0;
        for (size_t i = 0; i < queue.size(); i++)
        {
            const SectionPos &pos = queue[i];
            if (queuedSets[q]->find(pos) == queuedSets[q]->end())
            {
                continue;
            }

            if (!IsInMeshRadius(levelRenderer, pos, cameraPos))
            {
                ForgetSection(levelRenderer, pos);
                continue;
            }

            queue[kept++] = pos;
        }

        queue.resize(kept);
    }
}

void LevelChunkMeshingService::ScheduleMesher(LevelRenderer *levelRenderer)
{
    if (!levelRenderer->m_mesherRunning || !levelRenderer->m_mesherPool)
    {
        return;
    }

    // Finished meshes wait for the main thread to upload them, so producing faster than that only grows the heap
    size_t maxPendingUploads =
            (size_t) std::max(Everstead::GetInstance()->options.chunkUploadsPerFrame * 8, (int) levelRenderer->m_maxMesherTasks * 2);
    if (GetPendingMeshCount(levelRenderer) >= maxPendingUploads)
    {
        return;
    }

    bool smoothLighting              = Everstead::GetInstance()->options.smoothLighting;
    bool grassSideOverlay            = levelRenderer->m_grassSideOverlayEnabled;
    int maxMesherTasks               = (int) levelRenderer->m_maxMesherTasks;
    const glm::dvec3 &cameraPos      = Everstead::GetInstance()->GetCamera()->GetPosition();
    const glm::dvec3 &cameraFront    = Everstead::GetInstance()->GetCamera()->GetFront();
    const double behindCameraPenalty = 4.0;
    // Height counts less so flying still fills in the ground rings around the player first
    const double verticalWeight = 0.25;

    bool compacted = false;
    while (levelRenderer->m_activeMesherTasks.load() < maxMesherTasks)
    {
        SectionPos pos;
        bool haveTask       = false;
        bool stale          = false;
        bool urgent         = false;
        bool deferred       = false;
        uint64_t generation = 0;
        uint64_t syncGroup  = 0;

        {
            std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);

            std::vector<SectionPos> *queue = nullptr;
            if (!levelRenderer->m_urgentQueue.empty())
            {
                queue  = &levelRenderer->m_urgentQueue;
                urgent = true;
            }
            else if (!levelRenderer->m_rebuildQueue.empty())
            {
                queue = &levelRenderer->m_rebuildQueue;
            }

            if (queue)
            {
                size_t priorityIndex    = 0;
                double priorityDistance = std::numeric_limits<double>::max();
                for (size_t i = 0; i < queue->size(); i++)
                {
                    const SectionPos &candidate = (*queue)[i];
                    double dx                   = ((double) candidate.x + 0.5) * Chunk::s_sizeX - cameraPos.x;
                    double dy                   = ((double) candidate.y + 0.5) * ChunkSection::s_size - cameraPos.y;
                    double dz                   = ((double) candidate.z + 0.5) * Chunk::s_sizeZ - cameraPos.z;
                    double distance             = dx * dx + dy * dy * verticalWeight + dz * dz;
                    if (dx * cameraFront.x + dy * cameraFront.y + dz * cameraFront.z < 0.0)
                    {
                        distance *= behindCameraPenalty;
                    }

                    if (distance < priorityDistance)
                    {
                        priorityIndex    = i;
                        priorityDistance = distance;
                    }
                }

                pos                     = (*queue)[priorityIndex];
                (*queue)[priorityIndex] = queue->back();
                queue->pop_back();

                // DropChunkMeshes clears the queued sets but leaves the vector entries, which are skipped here
                size_t queued = urgent ? levelRenderer->m_urgentQueued.erase(pos) : levelRenderer->m_rebuildQueued.erase(pos);
                haveTask      = queued != 0;
                stale         = queued == 0;
                if (haveTask && !IsInMeshRadius(levelRenderer, pos, cameraPos))
                {
                    ForgetSection(levelRenderer, pos);
                    haveTask = false;
                    stale    = true;
                }
            }

            // Popping dead entries one at a time rescans the whole queue each time, so we drop them all in one pass
            if (stale && !compacted)
            {
                CompactRebuildQueues(levelRenderer, cameraPos);
                compacted = true;
            }

            if (haveTask)
            {
                if (levelRenderer->m_activeRebuilds.find(pos) != levelRenderer->m_activeRebuilds.end())
                {
                    if (urgent)
                    {
                        levelRenderer->m_deferredRebuilds.erase(pos);
                        levelRenderer->m_deferredUrgentRebuilds.insert(pos);
                    }
                    else if (levelRenderer->m_deferredUrgentRebuilds.find(pos) == levelRenderer->m_deferredUrgentRebuilds.end())
                    {
                        levelRenderer->m_deferredRebuilds.insert(pos);
                    }

                    haveTask = false;
                    deferred = true;
                }
                else
                {
                    auto generationIt = levelRenderer->m_requestedMeshGenerations.find(pos);
                    if (generationIt == levelRenderer->m_requestedMeshGenerations.end())
                    {
                        haveTask = false;
                        stale    = true;
                    }
                    else
                    {
                        levelRenderer->m_activeRebuilds.insert(pos);
                        generation       = generationIt->second;
                        auto syncGroupIt = levelRenderer->m_requestedMeshSyncGroups.find(pos);
                        syncGroup        = syncGroupIt != levelRenderer->m_requestedMeshSyncGroups.end() ? syncGroupIt->second : 0;
                    }
                }
            }
        }

        // A section that is still meshing was deferred, so we keep filling the free slots with the next nearest one
        if (stale || deferred)
        {
            continue;
        }

        if (!haveTask)
        {
            break;
        }

        const Chunk *chunk = levelRenderer->m_level->GetChunk(pos.ToChunkPos());
        if (!chunk)
        {
            std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
            levelRenderer->m_activeRebuilds.erase(pos);
            ForgetSection(levelRenderer, pos);
            continue;
        }

        levelRenderer->m_activeMesherTasks.fetch_add(1);

        levelRenderer->m_mesherPool->DetachTask(
                [levelRenderer, pos, chunk, smoothLighting, grassSideOverlay, generation, syncGroup, urgent]
                {
                    ThreadStorage::UseDefaultThreadStorage();
                    bool failed = false;
                    try
                    {
                        if (levelRenderer->m_mesherRunning)
                        {
                            std::vector<ChunkMesh::PackedData> packed;
                            BuildPackedSection(levelRenderer, chunk, pos, smoothLighting, grassSideOverlay, &packed);

                            SubmitMesh(levelRenderer, pos, generation, syncGroup, SectionVisibility::Compute(chunk, pos.y), std::move(packed));
                        }
                    }
                    catch (const std::bad_alloc &)
                    {
                        MemoryMonitor::GetInstance()->ReportOutOfMemory("chunk meshing");
                        failed = true;
                    }

                    {
                        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
                        levelRenderer->m_activeRebuilds.erase(pos);

                        // A lost result would leave its sync group waiting forever, so retry it once memory frees up
                        if (failed && levelRenderer->m_requestedMeshGenerations.find(pos) != levelRenderer->m_requestedMeshGenerations.end())
                        {
                            if (urgent)
                            {
                                levelRenderer->m_deferredRebuilds.erase(pos);
                                levelRenderer->m_deferredUrgentRebuilds.insert(pos);
                            }
                            else if (levelRenderer->m_deferredUrgentRebuilds.find(pos) == levelRenderer->m_deferredUrgentRebuilds.end())
                            {
                                levelRenderer->m_deferredRebuilds.insert(pos);
                            }
                        }

                        if (levelRenderer->m_deferredUrgentRebuilds.erase(pos) != 0)
                        {
                            if (levelRenderer->m_urgentQueued.insert(pos).second)
                            {
                                levelRenderer->m_urgentQueue.push_back(pos);
                            }
                        }
                        else if (levelRenderer->m_deferredRebuilds.erase(pos) != 0)
                        {
                            if (levelRenderer->m_rebuildQueued.insert(pos).second)
                            {
                                levelRenderer->m_rebuildQueue.push_back(pos);
                            }
                        }
                    }

                    levelRenderer->m_activeMesherTasks.fetch_sub(1);
                });
    }
}

void LevelChunkMeshingService::SubmitMesh(LevelRenderer *levelRenderer, const SectionPos &pos, uint64_t generation, uint64_t syncGroup,
                                          uint64_t visibility, std::vector<ChunkMesh::PackedData> &&packedResults)
{
    {
        std::lock_guard<std::mutex> lock(levelRenderer->m_rebuildQueueMutex);
        auto generationIt = levelRenderer->m_requestedMeshGenerations.find(pos);
        if (generationIt == levelRenderer->m_requestedMeshGenerations.end() || generation != generationIt->second)
        {
            return;
        }
    }

    std::lock_guard<std::mutex> lock(levelRenderer->m_meshQueueMutex);
    levelRenderer->m_pendingMeshes.push_back({pos, generation, syncGroup, visibility, std::move(packedResults)});
}

void LevelChunkMeshingService::RemoveChunkFromPendingSyncGroups(LevelRenderer *levelRenderer, const SectionPos &pos)
{
    // m_requestedMeshSyncGroups always names the one group a section belongs to, so no scan over every group is needed
    auto syncIt = levelRenderer->m_requestedMeshSyncGroups.find(pos);
    if (syncIt == levelRenderer->m_requestedMeshSyncGroups.end() || syncIt->second == 0)
    {
        return;
    }

    auto groupIt = levelRenderer->m_pendingSyncGroups.find(syncIt->second);
    if (groupIt == levelRenderer->m_pendingSyncGroups.end())
    {
        return;
    }

    groupIt->second.erase(pos);
    if (groupIt->second.empty())
    {
        levelRenderer->m_pendingSyncGroups.erase(groupIt);
    }
}

void LevelChunkMeshingService::ForgetSection(LevelRenderer *levelRenderer, const SectionPos &pos)
{
    RemoveChunkFromPendingSyncGroups(levelRenderer, pos);
    levelRenderer->m_requestedMeshGenerations.erase(pos);
    levelRenderer->m_requestedMeshSyncGroups.erase(pos);
    levelRenderer->m_rebuildQueued.erase(pos);
    levelRenderer->m_urgentQueued.erase(pos);
    levelRenderer->m_deferredRebuilds.erase(pos);
    levelRenderer->m_deferredUrgentRebuilds.erase(pos);
}

#endif
