#include "EversteadServer.h"

#include <algorithm>

#include "FrameProfiler.h"
#include "GameInfo.h"

#include "../entity/EntityTracker.h"
#include "../entity/Mob.h"
#include "../entity/item/Boat.h"
#include "../entity/item/Minecart.h"
#include "../entity/player/ServerPlayer.h"
#include "../network/PlayerList.h"
#include "../network/ServerConnection.h"
#include "../network/command/CommandContext.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/s2c/ServerStatusS2CPacket.h"
#include "../network/packet/s2c/TickingStateS2CPacket.h"
#include "../utils/StringUtils.h"
#include "../world/ServerLevel.h"
#include "../world/chunk/ChunkManager.h"
#include "../world/generation/TerrainGenerator.h"
#include "../world/save/WorldSave.h"
#include "Logger.h"

EversteadServer *EversteadServer::s_instance = nullptr;

EversteadServer *EversteadServer::GetInstance()
{
    if (!s_instance)
    {
        s_instance = new EversteadServer();
    }

    return s_instance;
}

void EversteadServer::DestroyInstance()
{
    delete s_instance;
    s_instance = nullptr;
}

EversteadServer::EversteadServer()
{
    m_levels          = {};
    m_playerList      = new PlayerList(this);
    m_connection      = new ServerConnection(this);
    m_worldName       = "";
    m_sid             = 0;
    m_maxPlayers      = 0;
    m_running         = false;
    m_saving          = false;
    m_nextEntityId    = 1;
    m_averageTickMs   = 0.0f;
    m_tps             = 20.0f;
    m_secondTicks     = 0;
    m_statusTicks     = 0;
    m_secondStart     = std::chrono::steady_clock::now();
    m_sprintTicks     = 0;
    m_sprintTicksLeft = 0;
    m_sprintWasFrozen = false;
    m_sprintStart     = m_secondStart;
}

EversteadServer::~EversteadServer()
{
    Stop();
    delete m_connection;
    delete m_playerList;
}

void EversteadServer::Start(ServerLevel *level, const std::string &worldName, Sid sid, int maxPlayers)
{
    Stop();
    m_tickRateManager = TickRateManager();
    m_sprintTicksLeft = 0;
    m_levels.push_back(level);
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (dynamic_cast<Mob *>(entity) || dynamic_cast<Boat *>(entity) || dynamic_cast<Minecart *>(entity))
        {
            m_playerList->GetEntityTracker()->Track(entity);
        }
    }

    m_worldName  = worldName;
    m_sid        = sid;
    m_maxPlayers = kStickFight ? std::min(maxPlayers, 4) : maxPlayers;
    m_running    = true;
    m_saving     = false;
    m_playerList->LoadBans();
    m_stickFight.Reset(this);
    Logger::LogInfo("Everstead server started world=%s sid=%llu maxPlayers=%d", m_worldName.c_str(), (unsigned long long) m_sid, m_maxPlayers);
}

void EversteadServer::Stop()
{
    m_connection->Clear();
    m_playerList->Clear();
    m_levels.clear();
    m_extraDimensions.clear();
    m_running = false;
    m_saving  = false;
}

void EversteadServer::Tick()
{
    if (!m_running)
    {
        return;
    }

    m_tickProfiler.BeginTick();
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    {
        FrameProfilerScope scope("Server.Connection");
        m_connection->Tick();
    }

    {
        FrameProfilerScope scope("Server.PlayerList");
        m_playerList->Tick();
    }

    if (!m_saving)
    {
        m_tickRateManager.Tick();
        for (size_t i = 0; i < m_levels.size(); i++)
        {
            m_levels[i]->Tick();
        }

        if (kStickFight)
        {
            m_stickFight.Tick();
        }
    }

    m_tickProfiler.EndTick();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    m_averageTickMs                           = m_averageTickMs * 0.9f + std::chrono::duration<float, std::milli>(end - start).count() * 0.1f;
    m_secondTicks++;
    float secondElapsed = std::chrono::duration<float>(end - m_secondStart).count();
    if (secondElapsed >= 1.0f)
    {
        m_tps         = IsSprinting() ? m_secondTicks / secondElapsed : std::min(m_secondTicks / secondElapsed, m_tickRateManager.GetTickRate());
        m_secondTicks = 0;
        m_secondStart = end;
    }

    if (IsSprinting())
    {
        m_sprintTicksLeft--;
        if (m_sprintTicksLeft == 0)
        {
            FinishSprint();
        }
    }

    m_statusTicks++;
    if (m_statusTicks >= 20)
    {
        m_statusTicks                                             = 0;
        const std::vector<std::unique_ptr<ServerPlayer>> &players = m_playerList->GetPlayers();
        std::vector<ServerStatusS2CPacket::PlayerInfo> infos(players.size());
        for (size_t i = 0; i < players.size(); i++)
        {
            infos[i].playerIndex = players[i]->GetPlayerIndex();
            infos[i].ping        = (uint16_t) std::min(players[i]->GetPlayerConnection()->GetConnection()->GetPing(), 65535);
            infos[i].isOperator  = players[i]->GetSharedFlag(eEntityFlag_Operator);
            infos[i].name        = StringUtils::ToUtf8(players[i]->GetName());
        }

        m_playerList->SendToAll(
                std::shared_ptr<Packet>(new ServerStatusS2CPacket(m_tps, m_averageTickMs, (uint8_t) std::min(m_maxPlayers, 255), infos)));
    }
}

void EversteadServer::SetSaving(bool saving)
{
    m_saving = saving;
}

int EversteadServer::NextEntityId()
{
    return m_nextEntityId++;
}

float EversteadServer::GetTps() const
{
    return m_tps;
}

TickRateManager *EversteadServer::GetTickRateManager()
{
    return &m_tickRateManager;
}

void EversteadServer::SendTickingState()
{
    m_playerList->SendToAll(std::shared_ptr<Packet>(new TickingStateS2CPacket(m_tickRateManager.GetTickRate(), m_tickRateManager.IsFrozen())));
}

bool EversteadServer::RequestSprint(int ticks)
{
    bool interrupted = IsSprinting();
    if (!interrupted)
    {
        m_sprintWasFrozen = m_tickRateManager.IsFrozen();
    }

    m_sprintTicks     = ticks;
    m_sprintTicksLeft = ticks;
    m_sprintStart     = std::chrono::steady_clock::now();
    m_tickRateManager.SetFrozen(false);
    SendTickingState();
    return interrupted;
}

bool EversteadServer::StopSprinting()
{
    if (!IsSprinting())
    {
        return false;
    }

    FinishSprint();
    return true;
}

void EversteadServer::FinishSprint()
{
    int ticks        = std::max(1, m_sprintTicks - m_sprintTicksLeft);
    double msPerTick = std::max(1.0, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - m_sprintStart).count()) / ticks;
    char tps[32];
    char mspt[32];
    snprintf(tps, sizeof(tps), "%.0f", 1000.0 / msPerTick);
    snprintf(mspt, sizeof(mspt), "%.2f", msPerTick);
    m_playerList->BroadcastChat(CommandContext::Translate("commands.tick.sprint.report", {StringUtils::ToUtf16(tps), StringUtils::ToUtf16(mspt)}));

    m_sprintTicks     = 0;
    m_sprintTicksLeft = 0;
    m_tickRateManager.SetFrozen(m_sprintWasFrozen);
    SendTickingState();
}

bool EversteadServer::IsSprinting() const
{
    return m_sprintTicksLeft > 0;
}

const TickProfiler &EversteadServer::GetTickProfiler() const
{
    return m_tickProfiler;
}

bool EversteadServer::IsRunning() const
{
    return m_running;
}

bool EversteadServer::IsSaving() const
{
    return m_saving;
}

ServerConnection *EversteadServer::GetConnection() const
{
    return m_connection;
}

PlayerList *EversteadServer::GetPlayerList() const
{
    return m_playerList;
}

StickFight *EversteadServer::GetStickFight()
{
    return &m_stickFight;
}

void EversteadServer::UpdateLighting()
{
    for (size_t i = 0; i < m_levels.size(); i++)
    {
        FrameProfilerScope scope("Server.UpdateLighting");
        m_levels[i]->UpdateLighting(400);
    }
}

ServerLevel *EversteadServer::GetOrCreateLevel(eDIMENSION_ID dimension)
{
    ServerLevel *existing = GetLevel(dimension);
    if (existing)
    {
        return existing;
    }

    ServerLevel *overworld                = GetLevel();
    std::unique_ptr<ExtraDimension> extra = std::make_unique<ExtraDimension>();
    extra->level                          = std::make_unique<ServerLevel>(this);
    ServerLevel *level                    = extra->level.get();
    WorldGenSettings genSettings          = overworld->GetGenSettings();
    genSettings.worldType                 = eWorldType_Caves;
    level->SetDimensionId(dimension);
    level->SetSpawnAreaRadius(overworld->GetSpawnAreaRadius());
    level->SetRenderDistance(overworld->GetRenderDistance());
    level->SetSimulationDistance(overworld->GetSimulationDistance());
    level->SetSeed(overworld->GetSeed());
    level->SetGenSettings(genSettings);
    level->SetDifficulty(overworld->GetDifficulty());
    level->SetDefaultGameType(overworld->GetDefaultGameType());
    level->ShareGameRules(overworld);
    level->ShareScoreboard(overworld);
    level->SetOnlineGame(overworld->IsOnlineGame());
    level->SetMaxPlayers(overworld->GetMaxPlayers());
    level->SetWorldTime(overworld->GetWorldProvider().GetWorldTime());
    level->SetWorldSpawn(TerrainGenerator(level->GetSeed(), genSettings).FindSpawn());
    WorldSave::LoadDimensionEntities(level, m_worldName);

    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (dynamic_cast<Mob *>(entity) || dynamic_cast<Boat *>(entity) || dynamic_cast<Minecart *>(entity))
        {
            m_playerList->GetEntityTracker()->Track(entity);
        }
    }

    extra->chunkManager = std::make_unique<ChunkManager>(level);
    level->SetChunkManager(extra->chunkManager.get());
    extra->chunkManager->SetWorldName(m_worldName);
    extra->chunkManager->Start();
    m_levels.push_back(level);
    m_extraDimensions.push_back(std::move(extra));
    Logger::LogInfo("Created dimension %s", kDimensionNames[dimension]);
    return level;
}

void EversteadServer::UpdateDimensions(SaveService *saveService, int unloadMargin)
{
    ServerLevel *overworld = GetLevel();
    std::vector<glm::dvec3> positions;
    for (size_t i = 0; i < m_extraDimensions.size(); i++)
    {
        ExtraDimension *extra = m_extraDimensions[i].get();
        extra->level->SetRenderDistance(overworld->GetRenderDistance());
        extra->level->SetSimulationDistance(overworld->GetSimulationDistance());
        m_playerList->GetPlayerPositions(extra->level.get(), &positions);
        extra->chunkManager->Update(positions);
        extra->chunkUnloader.UnloadServerChunks(extra->level.get(), positions, extra->level->GetRenderDistance() + unloadMargin, saveService,
                                                m_worldName);
    }
}

void EversteadServer::SaveDimensions()
{
    for (size_t i = 0; i < m_extraDimensions.size(); i++)
    {
        m_extraDimensions[i]->chunkManager->Stop();
        WorldSave::SaveDimension(m_extraDimensions[i]->level.get(), m_worldName, nullptr);
    }
}

ServerLevel *EversteadServer::GetLevel(eDIMENSION_ID dimension) const
{
    for (size_t i = 0; i < m_levels.size(); i++)
    {
        if (m_levels[i]->GetDimensionId() == dimension)
        {
            return m_levels[i];
        }
    }

    return nullptr;
}

const std::vector<ServerLevel *> &EversteadServer::GetLevels() const
{
    return m_levels;
}

const std::string &EversteadServer::GetWorldName() const
{
    return m_worldName;
}

Sid EversteadServer::GetSessionId() const
{
    return m_sid;
}

int EversteadServer::GetMaxPlayers() const
{
    return m_maxPlayers;
}

std::mutex &EversteadServer::GetMutex()
{
    return m_mutex;
}
