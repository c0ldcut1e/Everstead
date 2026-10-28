#include "PlayerChunkSender.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

#include "../core/EversteadServer.h"
#include "../core/Logger.h"
#include "../entity/player/ServerPlayer.h"
#include "../utils/math/Mth.h"
#include "../world/Level.h"
#include "../world/ServerLevel.h"
#include "../world/blockentity/MobSpawnerBlockEntity.h"
#include "../world/blockentity/SignBlockEntity.h"
#include "../world/chunk/Chunk.h"
#include "listener/PlayerConnection.h"
#include "packet/s2c/ChunkBlocksUpdateS2CPacket.h"
#include "packet/s2c/SignDataS2CPacket.h"
#include "packet/s2c/SpawnerDataS2CPacket.h"

PlayerChunkSender::PlayerChunkSender(ServerPlayer *player)
{
    m_player               = player;
    m_streamCenter         = ChunkPos(INT32_MAX, INT32_MAX);
    m_streamRenderDistance = -1;
    m_streamCursor         = 0;
    m_chunksInFlight       = 0;
    m_clientRenderDistance = INT32_MAX;
}

void PlayerChunkSender::Reset()
{
    m_sentChunks.clear();
    m_streamCenter = ChunkPos(INT32_MAX, INT32_MAX);
    m_streamCursor = 0;
}

void PlayerChunkSender::Tick()
{
    int chunksPerTick              = EversteadServer::GetInstance()->settings.chunkSendsPerTick;
    int maxChunksInFlight          = EversteadServer::GetInstance()->settings.chunkSendsInFlight;
    constexpr int kStreamSkipRings = 1;

    if (m_chunksInFlight >= maxChunksInFlight)
    {
        return;
    }

    ServerLevel *serverLevel = m_player->GetServer()->GetLevel(m_player->GetLevel()->GetDimensionId());
    if (!serverLevel)
    {
        return;
    }

    Level *world        = serverLevel;
    glm::dvec3 position = m_player->GetPosition();
    ChunkPos center(Mth::FloorDiv((int) floor(position.x), Chunk::s_sizeX), Mth::FloorDiv((int) floor(position.z), Chunk::s_sizeZ));
    // The client drops chunks past its own render distance, so we forget them too or they are never sent again
    int renderDistance      = std::min(world->GetRenderDistance(), m_clientRenderDistance);
    eDIMENSION_ID dimension = world->GetDimensionId();

    bool renderDistanceChanged = renderDistance != m_streamRenderDistance;
    if (renderDistanceChanged)
    {
        RebuildOffsets(renderDistance);
    }

    if (renderDistanceChanged || center != m_streamCenter)
    {
        m_streamCenter            = center;
        m_streamCursor            = 0;
        int renderDistanceSquared = renderDistance * renderDistance;
        for (auto sentIt = m_sentChunks.begin(); sentIt != m_sentChunks.end();)
        {
            int deltaX = sentIt->x - center.x;
            int deltaZ = sentIt->z - center.z;
            if (deltaX * deltaX + deltaZ * deltaZ > renderDistanceSquared)
            {
                sentIt = m_sentChunks.erase(sentIt);
            }
            else
            {
                ++sentIt;
            }
        }
    }

    while (m_streamCursor < m_streamOffsets.size())
    {
        const ChunkPos &offset = m_streamOffsets[m_streamCursor];
        if (m_sentChunks.count(ChunkPos(center.x + offset.x, center.z + offset.z)) == 0)
        {
            break;
        }

        m_streamCursor++;
    }

    PlayerConnection *connection = m_player->GetPlayerConnection();
    int sent                     = 0;
    int missingRadius            = -1;
    for (size_t i = m_streamCursor; i < m_streamOffsets.size() && sent < chunksPerTick; i++)
    {
        if (m_chunksInFlight >= maxChunksInFlight)
        {
            break;
        }

        const ChunkPos &offset = m_streamOffsets[i];
        int radius             = (int) sqrt((double) (offset.x * offset.x + offset.z * offset.z));
        if (missingRadius >= 0 && radius > missingRadius + kStreamSkipRings)
        {
            // We wait for closer chunks to generate instead of filling in far away ones first
            break;
        }

        ChunkPos pos(center.x + offset.x, center.z + offset.z);
        if (m_sentChunks.count(pos) > 0)
        {
            continue;
        }

        Chunk *chunk = world->GetChunk(pos);
        if (!chunk)
        {
            if (missingRadius < 0)
            {
                missingRadius = radius;
            }

            continue;
        }

        connection->Send(std::shared_ptr<Packet>(new ChunkBlocksUpdateS2CPacket(dimension, pos, *chunk)));
        for (const std::pair<const BlockPos, std::unique_ptr<BlockEntity>> &entry : world->GetBlockEntities())
        {
            if (Mth::FloorDiv(entry.first.x, Chunk::s_sizeX) != pos.x || Mth::FloorDiv(entry.first.z, Chunk::s_sizeZ) != pos.z)
            {
                continue;
            }

            MobSpawnerBlockEntity *spawner = dynamic_cast<MobSpawnerBlockEntity *>(entry.second.get());
            if (spawner)
            {
                connection->Send(std::shared_ptr<Packet>(new SpawnerDataS2CPacket(entry.first, spawner->GetEntityType())));
            }
            else if (dynamic_cast<SignBlockEntity *>(entry.second.get()))
            {
                std::unique_ptr<CompoundTag> tag(new CompoundTag());
                entry.second->Save(tag.get());
                connection->Send(std::shared_ptr<Packet>(new SignDataS2CPacket(entry.first, std::move(tag))));
            }
        }

        m_chunksInFlight++;
        m_sentChunks.insert(pos);
        sent++;
    }
}

bool PlayerChunkSender::Acknowledge(int count, int renderDistance)
{
    if (count < 0 || count > m_chunksInFlight || renderDistance <= 0)
    {
        Logger::LogWarn("Rejected chunk ack count=%d inFlight=%d renderDistance=%d", count, m_chunksInFlight, renderDistance);
        return false;
    }

    m_chunksInFlight -= count;
    m_clientRenderDistance = renderDistance;
    return true;
}

bool PlayerChunkSender::HasSentChunk(const ChunkPos &pos) const
{
    return m_sentChunks.count(pos) > 0;
}

void PlayerChunkSender::RebuildOffsets(int renderDistance)
{
    m_streamRenderDistance = renderDistance;
    m_streamOffsets.clear();

    int renderDistanceSquared = renderDistance * renderDistance;
    for (int x = -renderDistance; x <= renderDistance; x++)
    {
        for (int z = -renderDistance; z <= renderDistance; z++)
        {
            if (x * x + z * z <= renderDistanceSquared)
            {
                m_streamOffsets.push_back(ChunkPos(x, z));
            }
        }
    }

    std::sort(m_streamOffsets.begin(), m_streamOffsets.end(),
              [](const ChunkPos &a, const ChunkPos &b)
              {
                  return a.x * a.x + a.z * a.z < b.x * b.x + b.z * b.z;
              });
}
