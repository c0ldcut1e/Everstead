#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../utils/Random.h"
#include "Level.h"
#include "block/BlockPos.h"
#include "chunk/ChunkPos.h"
#include "particle/ParticleEngine.h"
#include "particle/ParticleSpawnParams.h"

class LevelRenderer;

class MultiPlayerLevel : public Level
{
public:
    MultiPlayerLevel();

    void SetLevelRenderer(LevelRenderer *levelRenderer);
    void Update(float partialTicks);
    ChunkPos GetPriorityChunk() const override;
    void ReceiveChunk(const ChunkPos &pos, std::unique_ptr<Chunk> chunk);

    ParticleEngine *GetParticleEngine() const;
    void SpawnParticle(const ParticleSpawnParams &params) override;

    bool IsClientSide() const override;
    Player *CreatePlayer(const std::wstring &name) override;
    Player *GetLocalPlayer() const override;
    void OnBlockLightChanged(const std::unordered_set<SectionPos, SectionPosHash> &sections, bool urgent) override;
    bool IsChunkMeshBusy(const ChunkPos &pos) const override;
    void ReleaseChunkMeshes(const ChunkPos &pos) override;

    void PlaySound(const ResourceLocation &location, const glm::dvec3 &position, float volume, float pitch) override;
    void PlaySound(const ResourceLocation &location, float volume, float pitch) override;
    void PlayThunder(const glm::dvec3 &position, float volume, float pitch) override;

    void Tick() override;
    TickRateManager *GetTickRateManager() const override;

protected:
    void TickWeather() override;
    void AdoptBlockEntities(const ChunkPos &pos) override;
    void OnSectionMeshDirty(const SectionPos &pos) override;
    bool IsClipTarget(const Entity *entity) const override;

private:
    struct PendingThunder
    {
        int ticks;
        float volume;
        float pitch;
    };

    void UpdateUrgentMeshes();
    void UpdateMeshes();
    void FlushMeshWaitingChunks();
    bool IsChunkReadyToMesh(const ChunkPos &pos) const;
    void AnimateTick();
    void TickRain();
    void TickThunder();

    LevelRenderer *m_levelRenderer;
    // Arrived chunks wait here until their neighbors are in, so they mesh once instead of once per neighbor
    std::unordered_map<ChunkPos, std::chrono::steady_clock::time_point, ChunkPosHash> m_meshWaitingChunks;
    std::unique_ptr<ParticleEngine> m_particleEngine;
    std::vector<ParticleSpawnParams> m_spawnScratch;
    int m_rainSoundTime;
    std::vector<PendingThunder> m_pendingThunder;
};
