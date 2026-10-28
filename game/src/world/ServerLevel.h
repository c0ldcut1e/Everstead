#pragma once

#include <deque>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../entity/EntityType.h"
#include "../utils/Random.h"
#include "Level.h"

class AbstractContainer;
class Block;
class EversteadServer;
class Entity;
class ItemEntity;
class Packet;
class PlayerConnection;
class ServerPlayer;

class ServerLevel : public Level
{
public:
    explicit ServerLevel(EversteadServer *server);

    void Tick() override;
    TickRateManager *GetTickRateManager() const override;

    void PlaySound(const ResourceLocation &location, const glm::dvec3 &position, float volume, float pitch) override;
    void RecordBlockChange(const BlockPos &pos) override;
    void ScheduleBlockForTick(const BlockPos &pos, uint32_t delayTicks, int priority) override;
    void AddBlockEvent(const BlockPos &pos, Block *block, int id) override;
    bool IsHandlingBlockTicks() const;
    size_t GetScheduledBlockTickCount() const override;
    void StoreBlockTicks(Chunk *chunk, bool remove) override;
    void OnWeatherChanged() override;
    void OnEntityHurt(Entity *entity) override;
    void OnMobDataChanged(Mob *mob) override;
    void BroadcastEntityEvent(Entity *entity, uint8_t event) override;
    void OnPassengersChanged(Entity *vehicle) override;
    void OnBoatDataChanged(Boat *boat) override;
    void AddEntity(std::unique_ptr<Entity> entity) override;
    Entity *GetEntityById(int id) const override;
    Player *FindNearestPlayer(const glm::dvec3 &position, double maxDistance) const override;
    bool CanMobSpawnAt(Entity *entity, const BlockPos &pos);
    void GetPushableEntities(const AABB &box, const Entity *except, std::vector<Entity *> *entities) const override;
    void RemoveEntity(Entity *entity) override;

    void SendEntitiesTo(ServerPlayer *player);
    void SendEntity(Entity *entity, PlayerConnection *connection);
    EversteadServer *GetServer() const;

    void DropContainerItems(const BlockPos &pos, AbstractContainer *container);
    void MarkBlockChunkDirty(const BlockPos &pos);
    void SendToNearby(const glm::dvec3 &position, double range, const std::shared_ptr<Packet> &packet);

protected:
    void TickWeather() override;
    void AdoptBlockEntities(const ChunkPos &pos) override;

private:
    struct SpawnEntry
    {
        eENTITY_TYPE type;
        int weight;
        int minCount;
        int maxCount;
    };

    struct ScheduledBlockTick
    {
        bool operator<(const ScheduledBlockTick &other) const
        {
            if (dueTick != other.dueTick)
            {
                return dueTick > other.dueTick;
            }

            if (priority != other.priority)
            {
                return priority > other.priority;
            }

            return sequence > other.sequence;
        }

        int64_t dueTick;
        int priority;
        int64_t sequence;
        BlockPos pos;
        Block *block;
    };

    struct BlockEvent
    {
        bool operator==(const BlockEvent &other) const
        {
            return pos == other.pos && block == other.block && id == other.id;
        }

        BlockPos pos;
        Block *block;
        int id;
    };

    void TickLightning();
    void TickSnow();
    void TickRandomBlocks();
    void TickMobSpawning();
    bool PickSpawnEntry(int category, Biome *biome, SpawnEntry *entry);
    bool SpawnMobAt(std::unique_ptr<Entity> entity, const BlockPos &pos);
    void SpawnChunkGenerationMobs(Chunk *chunk);
    void ProcessScheduledBlockTicks(int64_t nowTick);
    void UnparkBlockTicks();
    void RunBlockEvents();
    bool IsBlockTickScheduled(const ScheduledBlockTick &tick) const;
    void ForgetScheduledBlockTick(const ScheduledBlockTick &tick);
    void UpdateSimulationCenters();
    void BroadcastBlockChanges();
    void BroadcastRemovedEntities();
    void TickItems();
    bool TryPickup(ServerPlayer *player, ItemEntity *itemEntity);
    bool TryMerge(ItemEntity *itemEntity);
    void TickExperienceOrbs();

    EversteadServer *m_server;
    std::vector<BlockPos> m_blockChanges;
    std::unordered_set<BlockPos> m_blockChangeSet;
    std::vector<ItemEntity *> m_itemScratch;
    std::vector<Entity *> m_removedItemScratch;
    std::vector<int> m_removedEntityIds;
    std::vector<ChunkPos> m_simulationCenterScratch;
    std::vector<uint16_t> m_randomTickScratch;
    std::priority_queue<ScheduledBlockTick> m_scheduledBlockTicks;
    std::unordered_map<ChunkPos, std::unordered_map<BlockPos, ScheduledBlockTick>, ChunkPosHash> m_chunkBlockTicks;
    size_t m_scheduledBlockTickCount;
    std::unordered_map<ChunkPos, std::vector<ScheduledBlockTick>, ChunkPosHash> m_parkedBlockTicks;
    int64_t m_nextTickSequence;
    bool m_handlingBlockTicks;
    std::deque<BlockEvent> m_blockEvents;
    uint32_t m_lightningRandomValue;
};
