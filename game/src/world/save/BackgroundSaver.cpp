#include "BackgroundSaver.h"

#include <memory>

#include "../../core/EversteadServer.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../network/PlayerList.h"
#include "../../tag/CompoundTag.h"
#include "../../utils/math/Mth.h"
#include "../ServerLevel.h"
#include "../blockentity/BlockEntity.h"
#include "../chunk/Chunk.h"
#include "SaveService.h"
#include "WorldSave.h"

BackgroundSaver::BackgroundSaver()
{
    m_pendingChunks = {};
    m_scratch       = {};
}

void BackgroundSaver::Clear()
{
    m_pendingChunks.clear();
}

void BackgroundSaver::Begin(EversteadServer *server, SaveService *saveService, const std::string &worldName)
{
    std::shared_ptr<CompoundTag> levelTag = WorldSave::CreateLevelDataTag(server->GetLevel(), worldName);
    saveService->Enqueue(
            [worldName, levelTag]()
            {
                WorldSave::WriteLevelData(worldName, levelTag.get());
            });

    const std::vector<std::unique_ptr<ServerPlayer>> &players = server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        std::shared_ptr<CompoundTag> playerTag = WorldSave::CreatePlayerDataTag(players[i].get());
        std::string playerFileName             = WorldSave::GetPlayerDataFileName(players[i].get());
        saveService->Enqueue(
                [worldName, playerFileName, playerTag]()
                {
                    WorldSave::WritePlayerData(worldName, playerFileName, playerTag.get());
                });
    }

    m_pendingChunks.clear();
    const std::vector<ServerLevel *> &levels = server->GetLevels();
    for (size_t i = 0; i < levels.size(); i++)
    {
        levels[i]->CollectLoadedChunkPositions(&m_scratch);
        for (const ChunkPos &pos : m_scratch)
        {
            if (levels[i]->GetChunk(pos)->NeedsSave())
            {
                m_pendingChunks.push_back({levels[i], pos});
            }
        }

        // Container and furnace changes never dirty their chunk, so every chunk holding a block entity gets saved
        const std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> &blockEntities = levels[i]->GetBlockEntities();
        for (const std::pair<const BlockPos, std::unique_ptr<BlockEntity>> &blockEntity : blockEntities)
        {
            Chunk *chunk = levels[i]->GetChunk(
                    ChunkPos(Mth::FloorDiv(blockEntity.first.x, Chunk::s_sizeX), Mth::FloorDiv(blockEntity.first.z, Chunk::s_sizeZ)));
            if (chunk && !chunk->NeedsSave())
            {
                chunk->MarkDirty();
                m_pendingChunks.push_back({levels[i], chunk->GetPos()});
            }
        }
    }
}

void BackgroundSaver::Drain(SaveService *saveService, const std::string &worldName)
{
    int saved = 0;
    while (!m_pendingChunks.empty() && saved < s_maxChunksPerTick)
    {
        Level *level = m_pendingChunks.back().first;
        ChunkPos pos = m_pendingChunks.back().second;
        m_pendingChunks.pop_back();

        Chunk *chunk = level->GetChunk(pos);
        if (!chunk || !chunk->NeedsSave())
        {
            continue;
        }

        level->StoreBlockTicks(chunk, false);
        level->StoreBlockEntities(chunk, false);
        std::shared_ptr<Chunk> snapshot = std::make_shared<Chunk>(*chunk);
        chunk->SetBlockTicks({});
        chunk->SetBlockEntities({});
        chunk->SetSaved(true);
        chunk->ClearDirty();
        WorldSave::EnqueueChunkSave(saveService, worldName, level->GetDimensionId(), snapshot);
        saved++;
    }
}

bool BackgroundSaver::HasPendingChunks() const
{
    return !m_pendingChunks.empty();
}
