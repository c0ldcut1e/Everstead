#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../entity/player/GameType.h"
#include "../utils/HitResult.h"
#include "../utils/Random.h"
#include "DimensionId.h"
#include "DynamicLights.h"
#include "GameRules.h"
#include "WorldBorder.h"
#include "WorldProvider.h"
#include "block/BlockPos.h"
#include "blockentity/BlockEntity.h"
#include "chunk/Chunk.h"
#include "chunk/ChunkPos.h"
#include "chunk/SectionPos.h"
#include "generation/WorldGenSettings.h"
#include "item/ItemInstance.h"
#include "particle/ParticleSpawnParams.h"
#include "scoreboard/Scoreboard.h"

class Boat;
class ChunkManager;
class Entity;
class Mob;
class Player;
class TickRateManager;

class Level
{
public:
    Level();
    virtual ~Level();

    void SetChunkManager(ChunkManager *chunkManager);
    ChunkManager *GetChunkManager() const;

    void SetDimensionId(eDIMENSION_ID dimensionId);
    eDIMENSION_ID GetDimensionId() const;

    virtual void Tick();
    void TickTime();
    virtual TickRateManager *GetTickRateManager() const = 0;

    void UpdateChunks(int chunkIntakeBudget);
    void UpdateLighting(int budgetMicroseconds);
    virtual ChunkPos GetPriorityChunk() const;

    Chunk *GetChunk(const ChunkPos &pos);
    const Chunk *GetChunk(const ChunkPos &pos) const;
    Chunk &CreateChunk(const ChunkPos &pos);
    std::unique_ptr<Chunk> RemoveChunk(const ChunkPos &pos);
    bool HasChunk(const ChunkPos &pos) const;
    std::vector<std::pair<ChunkPos, Chunk *>> GetLoadedChunks() const;
    void CollectLoadedChunkPositions(std::unordered_set<ChunkPos, ChunkPosHash> *out) const;
    size_t GetChunkAllocatedBytes() const;
    size_t GetLoadedChunkCount() const;
    void MarkChunkDirty(const BlockPos &pos);
    void MarkChunkDirtyUrgent(const ChunkPos &pos);
    void ClearDirtyChunks();
    const std::deque<ChunkPos> &GetDirtyChunks() const;
    size_t GetQueuedDirtyChunkCount() const;
    size_t GetUrgentDirtyChunkCount() const;

    virtual void AddEntity(std::unique_ptr<Entity> entity);
    virtual void RemoveEntity(Entity *entity);
    void RemoveEntities(std::vector<Entity *> *entities);
    void TickEntities();
    void TickBlockEntities();
    const std::vector<std::unique_ptr<Entity>> &GetEntities() const;
    const std::vector<Entity *> &GetSolidEntities() const;
    virtual Entity *GetEntityById(int id) const;
    void UpdateEntityId(Entity *entity, int oldId, int newId);
    void EraseEntityId(Entity *entity);
    virtual Player *CreatePlayer(const std::wstring &name);
    virtual Player *GetLocalPlayer() const;
    virtual Player *FindNearestPlayer(const glm::dvec3 &position, double maxDistance) const;
    virtual void GetPushableEntities(const AABB &box, const Entity *except, std::vector<Entity *> *entities) const;
    void SpawnParticle(ePARTICLE_TYPE type, const glm::dvec3 &position);
    void SpawnParticle(ePARTICLE_TYPE type, const glm::dvec3 &position, int amount);
    void SpawnParticle(ePARTICLE_TYPE type, const glm::dvec3 &position, const glm::dvec3 &spread, int amount);
    virtual void SpawnParticle(const ParticleSpawnParams &params);
    virtual bool IsClientSide() const;
    GameRules &GetGameRules();
    void ShareGameRules(Level *level);
    Scoreboard &GetScoreboard();
    void ShareScoreboard(Level *level);
    virtual void PlaySound(const ResourceLocation &location, const glm::dvec3 &position, float volume, float pitch);
    virtual void PlaySound(const ResourceLocation &location, float volume, float pitch);
    virtual void PlayThunder(const glm::dvec3 &position, float volume, float pitch);
    void PlayBlockBreakSound(const BlockPos &pos, Block *block);
    void PlayBlockPlaceSound(const BlockPos &pos, Block *block);

    uint32_t GetBlockId(const BlockPos &pos) const;
    uint16_t GetBlockData(const BlockPos &pos) const;
    virtual void RecordBlockChange(const BlockPos &pos);
    void SetBlockData(const BlockPos &pos, uint16_t data);
    bool IsLiquidAt(const glm::dvec3 &position) const;
    bool IsLavaAt(const glm::dvec3 &position) const;
    bool IsLiquidAt(const glm::dvec3 &position, bool lava) const;
    void SetBlock(const BlockPos &pos, Block *block);
    void SetBlock(const BlockPos &pos, Block *block, Direction *placedAgainst);
    void SetBlock(const BlockPos &pos, Block *block, Direction *placedAgainst, uint16_t data);
    void SetBlockDeferredMesh(const BlockPos &pos, Block *block, uint16_t data);
    void SetBlockAndDataNoNeighborUpdates(const BlockPos &pos, Block *block, uint16_t data);
    void NotifyBlockNeighbors(const BlockPos &pos);
    void NotifyBlockAndNeighbors(const BlockPos &pos, const BlockPos &except);
    int GetDirectSignalTo(const BlockPos &pos) const;
    int GetSignal(const BlockPos &pos, Direction *direction) const;
    int GetBestNeighborSignal(const BlockPos &pos) const;
    virtual void ScheduleBlockForTick(const BlockPos &pos, uint32_t delayTicks, int priority);
    virtual void AddBlockEvent(const BlockPos &pos, Block *block, int id);
    virtual size_t GetScheduledBlockTickCount() const;
    virtual void StoreBlockTicks(Chunk *chunk, bool remove);
    BlockEntity *GetBlockEntity(const BlockPos &pos);
    const std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> &GetBlockEntities() const;
    void StoreBlockEntities(Chunk *chunk, bool remove);
    void RemoveBlockEntities(const ChunkPos &pos);
    WorldBorder &GetWorldBorder();
    DynamicLights &GetDynamicLights();
    void SetDifficulty(int difficulty);
    int GetDifficulty() const;
    void SetDefaultGameType(eGAME_TYPE gameType);
    eGAME_TYPE GetDefaultGameType() const;
    void SetOnlineGame(bool onlineGame);
    bool IsOnlineGame() const;
    void SetAllowFriendsOfFriends(bool allowFriendsOfFriends);
    bool AllowsFriendsOfFriends() const;
    void SetMaxPlayers(int maxPlayers);
    int GetMaxPlayers() const;
    void SetEmptyChunksSolid(bool solid);
    bool AreEmptyChunksSolid() const;
    void SetEmptyChunksSolidForMeshing(bool solid);
    bool AreEmptyChunksSolidForMeshing() const;

    int GetSurfaceHeight(int levelX, int levelZ) const;
    bool IntersectsBlock(const AABB &aabb) const;
    void GetCollisionBoxes(const AABB &aabb, std::vector<AABB> *boxes) const;
    bool ContainsLiquid(const AABB &aabb) const;
    HitResult *Clip(const glm::dvec3 &origin, const glm::dvec3 &direction, float maxDistance, bool includeEntities = true,
                    bool stopOnLiquidSource = false, bool collidableOnly = false);

    void SetRenderDistance(int distance);
    int GetRenderDistance() const;
    void SetSimulationDistance(int distance);
    void SetPlayerSimulationCenters(const std::vector<ChunkPos> &centers);
    int GetSimulationDistance() const;
    bool IsInSimulationDistance(const glm::dvec3 &position) const;
    void SetSpawnAreaRadius(int radius);
    void SetWorldSpawn(const BlockPos &pos);
    const BlockPos &GetWorldSpawn() const;
    bool HasWorldSpawn() const;
    glm::dvec3 FindPlayerSpawn(const BlockPos &center);
    void EnsureChunkLoaded(const ChunkPos &pos);
    int GetSpawnAreaRadius() const;
    Random &GetRandom();
    void SetSeed(uint32_t seed);
    uint32_t GetSeed() const;
    void SetGenSettings(const WorldGenSettings &settings);
    const WorldGenSettings &GetGenSettings() const;

    uint8_t GetLightLevel(const BlockPos &pos) const;
    int GetSkyDarken() const;
    int GetRawBrightness(const BlockPos &pos) const;
    float GetBrightness(const BlockPos &pos) const;
    uint8_t GetSkyLightLevel(const BlockPos &pos) const;
    uint8_t GetBlockLightLevel(const BlockPos &pos) const;
    void QueueLightUpdate(const BlockPos &pos);
    void QueueLightUpdateIfChanged(const BlockPos &pos, Block *oldBlock, Block *block);
    size_t GetQueuedLightUpdateCount() const;

    WorldProvider &GetWorldProvider();
    const WorldProvider &GetWorldProvider() const;
    void SetWorldTime(int64_t ticks);
    float GetCelestialAngle() const;
    uint8_t GetSkyLightClamp() const;

    void SetWeather(bool raining, int rainTime, bool thundering, int thunderTime);
    void SetWeatherLevels(float rainLevel, float thunderLevel);
    int64_t GetGameTime() const;
    bool IsRaining() const;
    bool IsThundering() const;
    int GetRainTime() const;
    int GetThunderTime() const;
    float GetRainLevel(float partialTicks) const;
    float GetThunderLevel(float partialTicks) const;
    bool IsRainingAt(const BlockPos &pos) const;
    bool IsSnowingAt(const BlockPos &pos) const;
    int GetRainHeight(int x, int z) const;
    virtual void OnWeatherChanged();
    virtual void OnEntityHurt(Entity *entity);
    virtual void OnMobDataChanged(Mob *mob);
    virtual void BroadcastEntityEvent(Entity *entity, uint8_t event);
    virtual void OnPassengersChanged(Entity *vehicle);
    virtual void OnBoatDataChanged(Boat *boat);
    virtual void OnBlockLightChanged(const std::unordered_set<SectionPos, SectionPosHash> &sections, bool urgent);
    virtual bool IsChunkMeshBusy(const ChunkPos &pos) const;
    virtual void ReleaseChunkMeshes(const ChunkPos &pos);
    void AddLightningFlash(const glm::dvec3 &position, float strength);
    float GetLightningFlash(float partialTicks) const;
    const glm::dvec3 &GetLightningFlashPosition() const;

    bool IsAreaLoaded(const BlockPos &pos, int radius) const;

protected:
    virtual void TickWeather();
    virtual void OnSectionMeshDirty(const SectionPos &pos);
    virtual bool IsClipTarget(const Entity *entity) const;
    bool AdoptChunk(const ChunkPos &pos, std::unique_ptr<Chunk> chunk);
    virtual void AdoptBlockEntities(const ChunkPos &pos);
    void ClearMeshDirtyQueues();
    bool PollDirtyChunk(ChunkPos *outPos, const ChunkPos &center);
    bool PollUrgentDirtyChunk(ChunkPos *outPos);
    bool PollUrgentDirtySections(std::vector<SectionPos> *outPositions);
    bool IsSkyLightSeamQueued(const ChunkPos &pos) const;

    std::vector<ChunkPos> m_simulationChunks;
    bool m_raining;
    bool m_thundering;
    int m_rainTime;
    int m_thunderTime;
    Random m_random;

private:
    void RecordLiquidChange(const BlockPos &pos, Block *oldBlock, Block *newBlock);

    static const int s_maxSpawnAreaRadius = 10;
    static const float s_breakSoundPitch;
    static const float s_placeSoundPitch;
    static const float s_placeSoundVolume;

    Chunk *FindChunk(const ChunkPos &pos) const;
    bool InsertChunk(const ChunkPos &pos, std::unique_ptr<Chunk> chunk);
    void MarkSectionsDirtyUrgent(const std::vector<SectionPos> &positions);
    bool PollLightUpdate(BlockPos *outPos);
    void QueueChunkSkyLightSeamUpdate(const ChunkPos &pos);
    bool PollSkyLightSeamUpdate(ChunkPos *outPos, const ChunkPos &center);
    void SetBlockAndData(const BlockPos &pos, Block *block, uint16_t data, bool urgentMesh);
    void OnBlockSet(const BlockPos &pos);
    void NotifyNeighborStates(const BlockPos &pos);
    void RunNeighborUpdates();
    void MarkBlockMeshDirty(const BlockPos &pos, bool urgent);
    void ScheduleWaterTicks(const ChunkPos &pos);
    void ScheduleSavedBlockTicks(const ChunkPos &pos);

    ChunkManager *m_chunkManager;
    eDIMENSION_ID m_dimensionId;
    std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> m_chunks;
    uint64_t m_id;
    std::atomic<uint64_t> m_removeEpoch;
    mutable std::shared_mutex m_chunksMutex;
    std::deque<ChunkPos> m_dirtyChunks;
    std::deque<ChunkPos> m_urgentDirtyChunks;
    std::deque<std::vector<SectionPos>> m_urgentDirtySections;
    std::unordered_set<ChunkPos, ChunkPosHash> m_dirtyChunksSet;
    std::unordered_set<ChunkPos, ChunkPosHash> m_urgentDirtyChunksSet;
    std::unordered_set<SectionPos, SectionPosHash> m_urgentDirtySectionsSet;
    mutable std::mutex m_dirtyMutex;
    std::deque<BlockPos> m_lightUpdates;
    std::deque<ChunkPos> m_skyLightSeamUpdates;
    std::unordered_set<ChunkPos, ChunkPosHash> m_skyLightSeamUpdatesSet;
    bool m_emptyChunksSolid;
    bool m_emptyChunksSolidForMeshing;
    int m_renderDistance;
    WorldProvider m_worldProvider;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::pair<BlockPos, uint64_t>> m_blockEntityTickOrder;
    uint64_t m_nextBlockEntityTickOrder;
    std::vector<BlockPos> m_neighborUpdates;
    bool m_updatingNeighbors;
    std::vector<std::unique_ptr<Entity>> m_pendingAddedEntities;
    std::vector<Entity *> m_pendingRemovedEntities;
    std::unordered_map<int, Entity *> m_entitiesById;
    std::vector<Entity *> m_solidEntities;
    std::vector<ChunkPos> m_simulationCenters;
    std::vector<ChunkPos> m_playerSimulationCenters;
    int m_simulationDistance;
    int64_t m_gameTime;
    GameRules m_ownGameRules;
    GameRules *m_gameRules;
    Scoreboard m_ownScoreboard;
    Scoreboard *m_scoreboard;
    WorldBorder m_worldBorder;
    DynamicLights m_dynamicLights;
    bool m_tickingEntities;
    int m_difficulty;
    eGAME_TYPE m_defaultGameType;
    bool m_onlineGame;
    bool m_allowFriendsOfFriends;
    int m_maxPlayers;
    int m_spawnAreaRadius;
    BlockPos m_worldSpawn;
    bool m_hasWorldSpawn;
    uint32_t m_seed;
    WorldGenSettings m_genSettings;
    float m_rainLevel;
    float m_oldRainLevel;
    float m_thunderLevel;
    float m_oldThunderLevel;
    int m_lightningFlashAge;
    float m_lightningFlashStrength;
    glm::dvec3 m_lightningFlashPosition;
};
