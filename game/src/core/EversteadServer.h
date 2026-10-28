#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../entity/EntityType.h"
#include "../network/NetworkTypes.h"
#include "../world/DimensionId.h"
#include "../world/StickFight.h"
#include "../world/chunk/ChunkUnloader.h"
#include "ServerSettings.h"
#include "TickProfiler.h"
#include "TickRateManager.h"

#include <glm/vec3.hpp>

class ChunkManager;
class PlayerList;
class SaveService;
class ServerConnection;
class ServerLevel;

class EversteadServer
{
public:
    static EversteadServer *GetInstance();
    static void DestroyInstance();

    EversteadServer();
    ~EversteadServer();

    void Start(ServerLevel *level, const std::string &worldName, Sid sid, int maxPlayers);
    void Stop();
    void Tick();
    void UpdateLighting();
    ServerLevel *GetOrCreateLevel(eDIMENSION_ID dimension);
    void UpdateDimensions(SaveService *saveService, int unloadMargin);
    void SaveDimensions();
    void SetSaving(bool saving);
    int NextEntityId();

    TickRateManager *GetTickRateManager();
    void SendTickingState();
    bool RequestSprint(int ticks);
    bool StopSprinting();
    bool IsSprinting() const;
    float GetTps() const;
    const TickProfiler &GetTickProfiler() const;
    bool IsRunning() const;
    bool IsSaving() const;
    ServerConnection *GetConnection() const;
    PlayerList *GetPlayerList() const;
    StickFight *GetStickFight();
    ServerLevel *GetLevel(eDIMENSION_ID dimension = eDimensionId_Overworld) const;
    const std::vector<ServerLevel *> &GetLevels() const;
    const std::string &GetWorldName() const;
    Sid GetSessionId() const;
    int GetMaxPlayers() const;
    std::mutex &GetMutex();

    ServerSettings settings;

private:
    void FinishSprint();

    static EversteadServer *s_instance;

    struct ExtraDimension
    {
        std::unique_ptr<ServerLevel> level;
        std::unique_ptr<ChunkManager> chunkManager;
        ChunkUnloader chunkUnloader;
    };

    std::vector<ServerLevel *> m_levels;
    std::vector<std::unique_ptr<ExtraDimension>> m_extraDimensions;
    PlayerList *m_playerList;
    StickFight m_stickFight;
    ServerConnection *m_connection;
    std::string m_worldName;
    Sid m_sid;
    int m_maxPlayers;
    std::atomic<bool> m_running;
    bool m_saving;
    int m_nextEntityId;
    float m_averageTickMs;
    float m_tps;
    TickProfiler m_tickProfiler;
    TickRateManager m_tickRateManager;
    int m_sprintTicks;
    int m_sprintTicksLeft;
    bool m_sprintWasFrozen;
    std::chrono::steady_clock::time_point m_sprintStart;
    int m_secondTicks;
    int m_statusTicks;
    std::chrono::steady_clock::time_point m_secondStart;
    std::mutex m_mutex;
};
