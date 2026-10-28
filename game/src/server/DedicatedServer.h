#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../io/File.h"
#include "../world/chunk/ChunkUnloader.h"
#include "../world/save/BackgroundSaver.h"
#include "../world/save/WorldLock.h"
#include "ServerConfig.h"
#include "ServerConsole.h"

class ChunkManager;
class SaveService;
class ServerLevel;
class ServerPlayer;
class ServerSession;

class DedicatedServer
{
public:
    explicit DedicatedServer(const File &configFile);
    ~DedicatedServer();

    int Run();

    static void RequestStop();

private:
    bool LoadWorld();
    void Tick();
    void BeginSave();
    void FinishSave();
    void Shutdown();
    void HandleCommand(const std::string &line);
    void HandleWhitelistCommand(const std::vector<std::string> &arguments);
    void SetOperator(const std::string &name, bool value);
    void SendChat(const nlohmann::json &component);
    ServerPlayer *FindPlayer(const std::string &name) const;

    static inline std::atomic<bool> s_stopRequested = false;

    File m_configFile;
    ServerConfig m_config;
    ServerConsole m_console;
    ServerSession *m_session;
    WorldLock m_worldLock;
    ServerLevel *m_level;
    ChunkManager *m_chunkManager;
    SaveService *m_saveService;
    BackgroundSaver m_backgroundSaver;
    ChunkUnloader m_chunkUnloader;
    glm::dvec3 m_spawnPosition;
    int m_ticksSinceAutosave;
    bool m_saveActive;
    uint32_t m_saveStartMs;
    double m_averageTickMs;
};
