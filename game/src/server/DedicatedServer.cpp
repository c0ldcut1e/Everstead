#ifdef EVERSTEAD_DEDICATED_SERVER

#include "DedicatedServer.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <thread>

#include "../core/EversteadServer.h"
#include "../core/FrameProfiler.h"
#include "../core/Localization.h"
#include "../core/Logger.h"
#include "../core/MemoryMonitor.h"
#include "../entity/EntityFactory.h"
#include "../entity/player/GameType.h"
#include "../entity/player/ServerPlayer.h"
#include "../io/FileSystem.h"
#include "../network/ApiClient.h"
#include "../network/PlayerList.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/s2c/ChatS2CPacket.h"
#include "../sound/SoundRegistry.h"
#include "../utils/StringUtils.h"
#include "../utils/Time.h"
#include "../world/ServerLevel.h"
#include "../world/biome/BiomeRegistry.h"
#include "../world/block/BlockRegistry.h"
#include "../world/chunk/ChunkManager.h"
#include "../world/crafting/FurnaceRecipes.h"
#include "../world/crafting/Recipes.h"
#include "../world/generation/TerrainGenerator.h"
#include "../world/generation/WorldSeed.h"
#include "../world/inventory/ServerContainerMenu.h"
#include "../world/item/ItemRegistry.h"
#include "../world/loot/LootTables.h"
#include "../world/particle/ParticleRegistry.h"
#include "../world/save/SaveService.h"
#include "../world/save/WorldLock.h"
#include "../world/save/WorldSave.h"
#include "ServerSession.h"

DedicatedServer::DedicatedServer(const File &configFile)
{
    m_configFile         = configFile;
    m_session            = nullptr;
    m_level              = nullptr;
    m_chunkManager       = nullptr;
    m_saveService        = nullptr;
    m_spawnPosition      = glm::dvec3(0.5, 135.0, 0.5);
    m_ticksSinceAutosave = 0;
    m_saveActive         = false;
    m_saveStartMs        = 0;
    m_averageTickMs      = 0.0;
}

DedicatedServer::~DedicatedServer()
{
    delete m_session;
}

int DedicatedServer::Run()
{
    if (!m_config.Load(m_configFile, &EversteadServer::GetInstance()->settings))
    {
        return 1;
    }

    EversteadServer::GetInstance()->settings.dedicated = true;
    MemoryMonitor::GetInstance()->ApplyLimit((uint64_t) m_config.memoryLimitMb);

    BlockRegistry::Init();
    ItemRegistry::Init();
    Recipes::Init();
    FurnaceRecipes::Init();
    LootTables::Init();
    ParticleRegistry::Init();
    BiomeRegistry::Init();
    EntityFactory::Init();
    SoundRegistry::Init();
    Localization::GetInstance()->ReloadLanguages();
    Localization::GetInstance()->LoadLanguage("en_us");

    m_saveService = new SaveService();
    m_saveService->Start();
    if (!LoadWorld())
    {
        m_saveService->Stop();
        return 1;
    }

    Sid sid = 0;
    if (m_config.online)
    {
        m_session = new ServerSession(m_config);
        if (!m_session->Start(m_config.maxPlayers))
        {
            Shutdown();
            return 1;
        }

        sid                                                 = m_session->GetSid();
        EversteadServer::GetInstance()->settings.serverName = m_session->GetName();
    }
    else
    {
        Logger::LogWarn("The server is offline, so nobody can join until session.online is turned on");
    }

    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        EversteadServer::GetInstance()->Start(m_level, m_config.worldName, sid, m_config.maxPlayers);
    }

    m_console.Start();
    Logger::LogInfo("Server started world=%s, type help for commands", m_config.worldName.c_str());

    std::chrono::steady_clock::time_point nextTick = std::chrono::steady_clock::now();
    while (!s_stopRequested)
    {
        std::chrono::steady_clock::time_point tickStart = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
            std::string line;
            while (m_console.PollLine(&line))
            {
                HandleCommand(line);
            }

            if (m_session)
            {
                m_session->Tick(EversteadServer::GetInstance()->GetPlayerList()->GetPlayerCount());
            }

            Tick();
            nextTick += EversteadServer::GetInstance()->GetTickRateManager()->GetTickInterval();
            if (EversteadServer::GetInstance()->IsSprinting())
            {
                nextTick = std::chrono::steady_clock::now();
            }
        }

        double tickMs   = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tickStart).count();
        m_averageTickMs = m_averageTickMs * 0.95 + tickMs * 0.05;

        if (m_session && m_session->HasExpired())
        {
            Logger::LogError("The session expired, stopping the server");
            break;
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        // We skip the missed ticks after a long stall instead of running them back to back
        if (now - nextTick > std::chrono::seconds(2))
        {
            Logger::LogWarn("Can't keep up, skipping %lld ticks",
                            (long long) ((now - nextTick) / EversteadServer::GetInstance()->GetTickRateManager()->GetTickInterval()));
            nextTick = now;
        }

        std::this_thread::sleep_until(nextTick);
    }

    Shutdown();
    return 0;
}

void DedicatedServer::RequestStop()
{
    s_stopRequested = true;
}

bool DedicatedServer::LoadWorld()
{
    WorldSave::SetSavesDirectory(m_configFile.GetParent().GetChild("worlds"));
    if (!m_worldLock.Acquire(WorldSave::GetWorldDirectory(m_config.worldName)))
    {
        Logger::LogError("World %s is already open in another server or game", m_config.worldName.c_str());
        return false;
    }

    m_level       = new ServerLevel(EversteadServer::GetInstance());
    bool existing = WorldSave::GetWorldDirectory(m_config.worldName).GetChild("level.dat").Exists();
    if (existing && !WorldSave::Load(m_level, m_config.worldName, nullptr))
    {
        Logger::LogError("Could not load world %s", m_config.worldName.c_str());
        delete m_level;
        m_level = nullptr;
        return false;
    }

    m_level->SetSpawnAreaRadius(m_config.viewDistance);
    m_level->SetRenderDistance(m_config.viewDistance);
    m_level->SetSimulationDistance(m_config.simulationDistance);
    m_level->SetOnlineGame(m_config.online);
    m_level->SetMaxPlayers(m_config.maxPlayers);
    if (!existing)
    {
        eGAME_TYPE gameType = eGameType_Survival;
        if (!GameType::Parse(m_config.gameMode, &gameType))
        {
            Logger::LogError("Unknown game mode %s", m_config.gameMode.c_str());
            delete m_level;
            m_level = nullptr;
            return false;
        }

        m_level->SetSeed(m_config.seed.empty() ? WorldSeed::Generate() : WorldSeed::Parse(m_config.seed));
        m_level->SetGenSettings(WorldGenSettings());
        m_level->SetDifficulty(std::clamp(m_config.difficulty, 0, 3));
        m_level->SetDefaultGameType(gameType);
        WorldSave::WriteLevelData(m_config.worldName, WorldSave::CreateLevelDataTag(m_level, m_config.worldName).get());
        Logger::LogInfo("Created world %s seed=%u", m_config.worldName.c_str(), m_level->GetSeed());
    }

    if (!m_level->HasWorldSpawn())
    {
        m_level->SetWorldSpawn(TerrainGenerator(m_level->GetSeed(), m_level->GetGenSettings()).FindSpawn());
    }

    const BlockPos &worldSpawn = m_level->GetWorldSpawn();
    m_spawnPosition            = glm::dvec3(worldSpawn.x + 0.5, worldSpawn.y, worldSpawn.z + 0.5);

    m_chunkManager = new ChunkManager(m_level);
    m_level->SetChunkManager(m_chunkManager);
    m_chunkManager->SetWorldName(m_config.worldName);
    m_chunkManager->Start();
    m_chunkManager->Update(m_spawnPosition);
    return true;
}

void DedicatedServer::Tick()
{
    FrameProfiler::BeginFrame();
    EversteadServer *server = EversteadServer::GetInstance();
    server->Tick();
    MemoryMonitor::GetInstance()->Tick();

    std::vector<glm::dvec3> positions;
    server->GetPlayerList()->GetPlayerPositions(m_level, &positions);
    if (positions.empty())
    {
        positions.push_back(m_spawnPosition);
    }

    {
        FrameProfilerScope scope("Dedicated.ChunkManager");
        m_chunkManager->Update(positions);
    }

    {
        FrameProfilerScope scope("Dedicated.UnloadChunks");
        m_chunkUnloader.UnloadServerChunks(m_level, positions, m_level->GetRenderDistance() + m_config.chunkUnloadMargin, m_saveService,
                                           m_config.worldName);
    }

    {
        FrameProfilerScope scope("Dedicated.Dimensions");
        server->UpdateDimensions(m_saveService, m_config.chunkUnloadMargin);
    }

    server->UpdateLighting();

    if (m_config.autosaveIntervalTicks > 0 && ++m_ticksSinceAutosave >= m_config.autosaveIntervalTicks)
    {
        BeginSave();
    }

    {
        FrameProfilerScope scope("Dedicated.Save");
        m_backgroundSaver.Drain(m_saveService, m_config.worldName);
    }

    if (m_saveActive && !m_saveService->IsBusy() && !m_backgroundSaver.HasPendingChunks())
    {
        FinishSave();
    }

    FrameProfiler::EndFrame();
}

void DedicatedServer::BeginSave()
{
    if (!m_saveActive)
    {
        m_saveStartMs = Time::GetMilliseconds();
    }

    m_ticksSinceAutosave = 0;
    m_saveActive         = true;
    m_backgroundSaver.Begin(EversteadServer::GetInstance(), m_saveService, m_config.worldName);
}

void DedicatedServer::FinishSave()
{
    m_saveActive = false;
    m_config.Save(m_configFile, EversteadServer::GetInstance()->settings);
    uint32_t elapsedMs = Time::GetMilliseconds() - m_saveStartMs;
    Logger::LogInfo("Saved the world in %ums", elapsedMs);
    if (!m_config.announceWorldSaves)
    {
        return;
    }

    char durationText[32];
    if (elapsedMs >= 1000)
    {
        snprintf(durationText, sizeof(durationText), "%.2fs", (double) elapsedMs / 1000.0);
    }
    else
    {
        snprintf(durationText, sizeof(durationText), "%ums", elapsedMs);
    }

    nlohmann::json durationComponent;
    durationComponent["text"] = durationText;

    nlohmann::json component;
    component["translate"] = "multiplayer.world.saved";
    component["color"]     = "gray";
    component["with"]      = nlohmann::json::array();
    component["with"].push_back(durationComponent);
    SendChat(component);
}

void DedicatedServer::Shutdown()
{
    Logger::LogInfo("Stopping the server");
    m_chunkManager->Stop();

    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        PlayerList *playerList                                    = EversteadServer::GetInstance()->GetPlayerList();
        const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
        for (size_t i = 0; i < players.size(); i++)
        {
            players[i]->GetContainerMenu()->CloseAll();
        }

        m_saveService->Flush();
        WorldSave::Save(m_level, m_config.worldName, playerList, nullptr);
        EversteadServer::GetInstance()->SaveDimensions();
        m_backgroundSaver.Clear();
        if (m_session)
        {
            m_session->Stop();
        }

        playerList->DisconnectAll(eDisconnectReason_ServerStopping);
        EversteadServer::GetInstance()->Stop();
    }

    m_saveService->Stop();
    delete m_chunkManager;
    delete m_level;
    delete m_saveService;
    m_chunkManager = nullptr;
    m_level        = nullptr;
    m_saveService  = nullptr;
    Logger::LogInfo("Server stopped");
}

void DedicatedServer::HandleCommand(const std::string &line)
{
    std::istringstream stream(line);
    std::vector<std::string> arguments;
    std::string argument;
    while (stream >> argument)
    {
        arguments.push_back(argument);
    }

    if (arguments.empty())
    {
        return;
    }

    std::string command = arguments[0];
    if (!command.empty() && command[0] == '/')
    {
        command.erase(0, 1);
    }

    PlayerList *playerList = EversteadServer::GetInstance()->GetPlayerList();
    if (command == "help")
    {
        Logger::LogInfo("Commands: stop, save, list, say <message>, kick <player>, ban <player>, pardon <player>, op <player>, deop <player>, "
                        "whitelist <on|off|add|remove|list> [player], tps, profile <ms|off>");
    }
    else if (command == "stop")
    {
        RequestStop();
    }
    else if (command == "save")
    {
        BeginSave();
    }
    else if (command == "list")
    {
        const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
        std::string names;
        for (size_t i = 0; i < players.size(); i++)
        {
            names += (i > 0 ? ", " : "") + StringUtils::ToUtf8(players[i]->GetName());
        }

        Logger::LogInfo("There are %d of %d players online: %s", playerList->GetPlayerCount(), m_config.maxPlayers, names.c_str());
    }
    else if (command == "say" && arguments.size() > 1)
    {
        nlohmann::json nameComponent;
        nameComponent["text"] = "Server";

        nlohmann::json messageComponent;
        messageComponent["text"] = line.substr(line.find(arguments[0]) + arguments[0].size() + 1);

        nlohmann::json component;
        component["translate"] = "chat.type.text";
        component["with"]      = nlohmann::json::array({nameComponent, messageComponent});
        SendChat(component);
        Logger::LogInfo("<Server> %s", messageComponent["text"].get<std::string>().c_str());
    }
    else if ((command == "kick" || command == "ban") && arguments.size() > 1)
    {
        ServerPlayer *player = FindPlayer(arguments[1]);
        if (!player)
        {
            Logger::LogWarn("No player named %s is online", arguments[1].c_str());
            return;
        }

        if (command == "kick")
        {
            player->GetPlayerConnection()->Disconnect(eDisconnectReason_Kicked);
            Logger::LogInfo("Kicked %s", arguments[1].c_str());
            return;
        }

        playerList->AddBanned(player->GetUniqueID(), StringUtils::ToUtf8(player->GetName()));

        nlohmann::json nameComponent;
        nameComponent["text"] = StringUtils::ToUtf8(player->GetName());

        nlohmann::json component;
        component["translate"] = "multiplayer.player.banned";
        component["color"]     = "yellow";
        component["with"]      = nlohmann::json::array({nameComponent});
        SendChat(component);

        player->GetPlayerConnection()->Disconnect(eDisconnectReason_Banned);
        Logger::LogInfo("Banned %s", arguments[1].c_str());
    }
    else if (command == "pardon" && arguments.size() > 1)
    {
        if (!playerList->RemoveBanned(arguments[1]))
        {
            Logger::LogWarn("%s is not banned", arguments[1].c_str());
            return;
        }

        Logger::LogInfo("Unbanned %s", arguments[1].c_str());
    }
    else if ((command == "op" || command == "deop") && arguments.size() > 1)
    {
        SetOperator(arguments[1], command == "op");
    }
    else if (command == "whitelist" && arguments.size() > 1)
    {
        HandleWhitelistCommand(arguments);
    }
    else if (command == "tps")
    {
        Logger::LogInfo("TPS %.1f, average tick %.2fms", std::min(20.0, 1000.0 / std::max(m_averageTickMs, 50.0)), m_averageTickMs);
        Logger::LogInfo("Chunks pending=%zu active=%zu", m_chunkManager->GetPendingCount(), m_chunkManager->GetActiveCount());
        const std::vector<ServerLevel *> &levels = EversteadServer::GetInstance()->GetLevels();
        for (size_t i = 0; i < levels.size(); i++)
        {
            Logger::LogInfo("Dimension %d entities=%zu chunks=%zu blockTicks=%zu lightUpdates=%zu", (int) levels[i]->GetDimensionId(),
                            levels[i]->GetEntities().size(), levels[i]->GetLoadedChunkCount(), levels[i]->GetScheduledBlockTickCount(),
                            levels[i]->GetQueuedLightUpdateCount());
        }
    }
    else if (command == "profile" && arguments.size() > 1)
    {
        bool enabled = arguments[1] != "off";
        if (enabled)
        {
            FrameProfiler::SetThresholdMs(std::max(1.0, atof(arguments[1].c_str())));
        }

        FrameProfiler::SetLogSlowFrames(enabled);
        FrameProfiler::SetEnabled(enabled);
        Logger::LogInfo("Tick profiling %s, threshold %.1fms", enabled ? "on" : "off", FrameProfiler::GetThresholdMs());
    }
    else
    {
        Logger::LogWarn("Unknown command %s, type help for a list", command.c_str());
    }
}

void DedicatedServer::HandleWhitelistCommand(const std::vector<std::string> &arguments)
{
    ServerSettings &settings = EversteadServer::GetInstance()->settings;
    auto entry = arguments.size() > 2 ? std::find(settings.whitelist.begin(), settings.whitelist.end(), arguments[2]) : settings.whitelist.end();
    if (arguments[1] == "on" || arguments[1] == "off")
    {
        settings.whitelistEnabled = arguments[1] == "on";
        Logger::LogInfo("The whitelist is now %s", arguments[1].c_str());
    }
    else if (arguments[1] == "add" && arguments.size() > 2)
    {
        if (entry == settings.whitelist.end())
        {
            settings.whitelist.push_back(arguments[2]);
        }

        Logger::LogInfo("Added %s to the whitelist", arguments[2].c_str());
    }
    else if (arguments[1] == "remove" && entry != settings.whitelist.end())
    {
        settings.whitelist.erase(entry);
        Logger::LogInfo("Removed %s from the whitelist", arguments[2].c_str());
    }
    else if (arguments[1] == "list")
    {
        std::string names;
        for (size_t i = 0; i < settings.whitelist.size(); i++)
        {
            names += (i > 0 ? ", " : "") + settings.whitelist[i];
        }

        Logger::LogInfo("Whitelist (%s): %s", settings.whitelistEnabled ? "on" : "off", names.c_str());
        return;
    }
    else
    {
        Logger::LogWarn("Usage: whitelist <on|off|add|remove|list> [player]");
        return;
    }

    m_config.Save(m_configFile, settings);
}

void DedicatedServer::SetOperator(const std::string &name, bool value)
{
    ServerSettings &settings = EversteadServer::GetInstance()->settings;
    ServerPlayer *player     = FindPlayer(name);
    if (value)
    {
        ServerOperator entry = {0, name};
        if (player)
        {
            entry.pid = player->GetPid();
        }
        else
        {
            ApiClient apiClient;
            apiClient.SetAccessToken(m_config.token);
            nlohmann::json document;
            NetworkResult result = apiClient.Get("/server/users/name/" + name, &document);
            if (result.IsFailure())
            {
                Logger::LogWarn("Could not find the account %s code=%s", name.c_str(), result.Format().c_str());
                return;
            }

            entry = {document.at("pid").get<uint32_t>(), document.at("username").get<std::string>()};
        }

        if (settings.IsOperator(entry.pid))
        {
            Logger::LogWarn("%s is already an operator", entry.name.c_str());
            return;
        }

        settings.operators.push_back(entry);
    }
    else
    {
        size_t count = settings.operators.size();
        for (size_t i = settings.operators.size(); i-- > 0;)
        {
            if ((player && settings.operators[i].pid == player->GetPid()) || StringUtils::EqualsIgnoreCase(settings.operators[i].name, name))
            {
                settings.operators.erase(settings.operators.begin() + i);
            }
        }

        if (settings.operators.size() == count)
        {
            Logger::LogWarn("%s is not an operator", name.c_str());
            return;
        }
    }

    m_config.Save(m_configFile, settings);
    Logger::LogInfo("%s %s", value ? "Made an operator of" : "Removed operator from", name.c_str());

    if (!player || player->IsOperator() == value)
    {
        return;
    }

    player->SetOperator(value);
    EversteadServer::GetInstance()->GetPlayerList()->BroadcastPlayerFlags(player);

    nlohmann::json component;
    component["translate"] = value ? "player.operator_granted" : "player.operator_revoked";
    player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ChatS2CPacket(component.dump(), true)));
}

void DedicatedServer::SendChat(const nlohmann::json &component)
{
    EversteadServer::GetInstance()->GetPlayerList()->BroadcastChat(component);
}

ServerPlayer *DedicatedServer::FindPlayer(const std::string &name) const
{
    const std::vector<std::unique_ptr<ServerPlayer>> &players = EversteadServer::GetInstance()->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (StringUtils::ToUtf8(players[i]->GetName()) == name)
        {
            return players[i].get();
        }
    }

    return nullptr;
}

#endif
