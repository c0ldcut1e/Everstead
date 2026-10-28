#ifndef EVERSTEAD_DEDICATED_SERVER

#include "EversteadWorldLoader.h"
#include "../utils/Time.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <thread>

#include <SDL2/SDL.h>

#include "../entity/Entity.h"
#include "../entity/player/MultiPlayerLocalPlayer.h"
#include "../network/MultiplayerSession.h"
#include "../network/listener/ClientConnection.h"
#include "../ui/scene/UIScene_WorldLoading.h"
#include "../utils/UniqueID.h"
#include "../utils/math/Mth.h"
#include "../world/MultiPlayerLevel.h"
#include "../world/ServerLevel.h"
#include "../world/block/Block.h"
#include "../world/chunk/Chunk.h"
#include "../world/generation/TerrainGenerator.h"
#include "../world/generation/WorldSeed.h"
#include "../world/save/WorldSave.h"
#include "Everstead.h"
#include "EversteadServer.h"

void EversteadWorldLoader::StartWorldLoad(Everstead *everstead)
{
    StartNewWorld(everstead, "New World", "", 2, eGameType_Survival);
}

void EversteadWorldLoader::StartNewWorld(Everstead *everstead, const std::string &worldName, const std::string &seed, int difficulty, int gameType)
{
    if (everstead->m_loadingPhase != Everstead::eLoadingPhase_None || everstead->m_inWorld)
    {
        return;
    }

    everstead->m_loadingPhase         = Everstead::eLoadingPhase_FindingSeed;
    everstead->m_loadingTicks         = 0;
    everstead->m_worldSeed            = 0;
    everstead->m_newWorldDifficulty   = difficulty;
    everstead->m_newWorldGameType     = gameType;
    everstead->m_newWorldSeed         = seed;
    everstead->m_loadingExistingWorld = false;
    everstead->m_loadingOnlineWorld   = everstead->options.onlineGame && everstead->GetAccountService()->IsLoggedIn();
    everstead->m_joiningOnlineWorld   = false;
    everstead->m_joinSessionId        = 0;
    everstead->m_worldName            = WorldSave::GetAvailableWorldName(worldName);
    everstead->m_worldLoadName.clear();

    UIScene_WorldLoading *worldLoadingScene = (UIScene_WorldLoading *) everstead->m_uiController->NavigateToScene(eUIScene_WorldLoading);
    if (worldLoadingScene)
    {
        worldLoadingScene->SetTitle(everstead->GetString("loading.creating_world"));
        worldLoadingScene->SetStatus(everstead->GetString("loading.picking_world_seed"));
        worldLoadingScene->SetProgress(0.0f);
    }
}

void EversteadWorldLoader::StartJoinedWorld(Everstead *everstead, uint64_t sid, const std::string &worldName)
{
    if (everstead->m_loadingPhase != Everstead::eLoadingPhase_None || everstead->m_inWorld)
    {
        return;
    }

    everstead->m_loadingPhase         = Everstead::eLoadingPhase_FindingSeed;
    everstead->m_loadingTicks         = 0;
    everstead->m_worldSeed            = 0;
    everstead->m_loadingExistingWorld = false;
    everstead->m_loadingOnlineWorld   = true;
    everstead->m_joiningOnlineWorld   = true;
    everstead->m_joinSessionId        = sid;
    everstead->m_worldName            = worldName.empty() ? "Joined World" : worldName;
    everstead->m_worldLoadName.clear();

    UIScene_WorldLoading *worldLoadingScene = (UIScene_WorldLoading *) everstead->m_uiController->NavigateToScene(eUIScene_WorldLoading);
    if (worldLoadingScene)
    {
        worldLoadingScene->SetTitle(everstead->GetString("loading.joining_world"));
        worldLoadingScene->SetStatus(everstead->GetString("loading.preparing_connection"));
        worldLoadingScene->SetProgress(0.0f);
    }
}

void EversteadWorldLoader::StartSavedWorld(Everstead *everstead, const std::string &worldName, bool online)
{
    if (everstead->m_loadingPhase != Everstead::eLoadingPhase_None || everstead->m_inWorld)
    {
        return;
    }

    if (!everstead->m_worldLock.Acquire(WorldSave::GetWorldDirectory(worldName)))
    {
        UIScene_ModalDialog::Dialog dialog;
        dialog.title   = everstead->GetString("world.locked.title");
        dialog.message = everstead->GetString("world.locked.message");
        UIScene_ModalDialog::Button button;
        button.label = everstead->GetString("button.ok");
        dialog.buttons.push_back(button);
        everstead->m_uiController->ShowModal(dialog);
        return;
    }

    everstead->m_loadingPhase         = Everstead::eLoadingPhase_LoadingSaveData;
    everstead->m_loadingTicks         = 0;
    everstead->m_worldSeed            = 0;
    everstead->m_loadingExistingWorld = true;
    everstead->m_loadingOnlineWorld   = online;
    everstead->m_joiningOnlineWorld   = false;
    everstead->m_joinSessionId        = 0;
    everstead->m_worldName            = worldName;
    everstead->m_worldLoadName        = worldName;
    everstead->m_loadFinished.store(false);
    everstead->m_loadSucceeded.store(false);
    everstead->m_loadProgress.store(0.0f);
    everstead->m_playerSpawnPosition = glm::dvec3(0.5, 135.0, 0.5);

    everstead->m_worldLevel = new ServerLevel(EversteadServer::GetInstance());
    everstead->m_loadThread = std::thread(
            [everstead, worldName]
            {
                bool loaded = WorldSave::Load(everstead->m_worldLevel, worldName, &everstead->m_loadProgress);
                everstead->m_loadSucceeded.store(loaded);
                everstead->m_loadFinished.store(true);
            });

    UIScene_WorldLoading *worldLoadingScene = (UIScene_WorldLoading *) everstead->m_uiController->NavigateToScene(eUIScene_WorldLoading);
    if (worldLoadingScene)
    {
        worldLoadingScene->SetTitle(everstead->GetString("loading.loading_world"));
        worldLoadingScene->SetStatus(everstead->GetString("loading.reading_save_data"));
        worldLoadingScene->SetProgress(0.0f);
    }
}

bool EversteadWorldLoader::IsWorldLoading(const Everstead *everstead)
{
    return everstead->m_loadingPhase != Everstead::eLoadingPhase_None;
}

void EversteadWorldLoader::BeginClientWorldInit(Everstead *everstead)
{
    everstead->SyncRenderSizeFromWindow();

    everstead->m_playerSpawnPosition = glm::dvec3(0.5, 135.0, 0.5);
    everstead->m_level               = new MultiPlayerLevel();
    everstead->m_level->SetSpawnAreaRadius(everstead->options.renderDistance);
    everstead->m_level->SetRenderDistance(everstead->options.renderDistance);
    everstead->m_level->SetEmptyChunksSolid(true);
    everstead->m_level->SetEmptyChunksSolidForMeshing(false);

    everstead->m_camera        = new Camera();
    everstead->m_levelRenderer = new LevelRenderer(everstead->m_level, everstead->m_width, everstead->m_height);
    everstead->m_level->SetLevelRenderer(everstead->m_levelRenderer);

    everstead->m_localPlayer = new MultiPlayerLocalPlayer(everstead->m_level, L"");
    everstead->m_localPlayer->SetPosition(everstead->m_playerSpawnPosition);
    everstead->m_localPlayer->Update(1.0f);
    everstead->m_level->AddEntity(std::unique_ptr<Entity>(everstead->m_localPlayer));
}

void EversteadWorldLoader::BeginWorldInit(Everstead *everstead)
{
    everstead->SyncRenderSizeFromWindow();

    if (!everstead->m_worldLevel)
    {
        everstead->m_worldLevel          = new ServerLevel(EversteadServer::GetInstance());
        everstead->m_playerSpawnPosition = glm::dvec3(0.5, 135.0, 0.5);
    }

    everstead->m_worldLevel->SetSpawnAreaRadius(everstead->options.renderDistance);
    everstead->m_worldLevel->SetRenderDistance(everstead->options.renderDistance);
    everstead->m_worldLevel->SetSimulationDistance(everstead->options.simulationDistance);
    if (!everstead->m_loadingExistingWorld)
    {
        everstead->m_worldLevel->SetSeed(everstead->m_worldSeed);
        everstead->m_worldLevel->SetGenSettings(everstead->m_newWorldGenSettings);
    }

    if (!everstead->m_worldLevel->HasWorldSpawn())
    {
        TerrainGenerator generator(everstead->m_worldLevel->GetSeed(), everstead->m_worldLevel->GetGenSettings());
        everstead->m_worldLevel->SetWorldSpawn(generator.FindSpawn());
    }

    const BlockPos &worldSpawn       = everstead->m_worldLevel->GetWorldSpawn();
    everstead->m_playerSpawnPosition = glm::dvec3(worldSpawn.x + 0.5, worldSpawn.y, worldSpawn.z + 0.5);

    if (everstead->m_loadingExistingWorld)
    {
        everstead->m_worldSeed = everstead->m_worldLevel->GetSeed();

        std::string savedWorldName = everstead->m_worldName.empty() ? "New World" : everstead->m_worldName;
        UniqueID localUniqueId     = everstead->m_multiplayerSession->GetPlayerUniqueID(everstead->m_loadingOnlineWorld);
        glm::dvec3 savedSpawn;
        if (WorldSave::ReadPlayerSpawnPosition(savedWorldName, localUniqueId, &savedSpawn))
        {
            everstead->m_playerSpawnPosition = savedSpawn;
        }
    }
    else
    {
        everstead->m_worldLevel->SetSeed(everstead->m_worldSeed);
        everstead->m_worldLevel->SetDifficulty(everstead->m_newWorldDifficulty);
        everstead->m_worldLevel->SetDefaultGameType((eGAME_TYPE) everstead->m_newWorldGameType);
        everstead->m_worldLevel->GetGameRules() = everstead->m_newWorldGameRules;
        everstead->m_worldLevel->SetOnlineGame(everstead->options.onlineGame);
        everstead->m_worldLevel->SetAllowFriendsOfFriends(everstead->options.allowFriendsOfFriends);
        everstead->m_worldLevel->SetMaxPlayers(everstead->options.maxPlayers);

        std::string newWorldName              = everstead->m_worldName.empty() ? "New World" : everstead->m_worldName;
        std::shared_ptr<CompoundTag> levelTag = WorldSave::CreateLevelDataTag(everstead->m_worldLevel, newWorldName);
        WorldSave::WriteLevelData(newWorldName, levelTag.get());
        everstead->m_worldLock.Acquire(WorldSave::GetWorldDirectory(newWorldName));
    }

    everstead->m_chunkManager = new ChunkManager(everstead->m_worldLevel);
    everstead->m_worldLevel->SetChunkManager(everstead->m_chunkManager);
    everstead->m_chunkManager->SetWorldName(everstead->m_worldName.empty() ? "New World" : everstead->m_worldName);
    everstead->ApplyServerSettings();
    everstead->m_chunkManager->Start();

    everstead->m_level = new MultiPlayerLevel();
    everstead->m_level->SetSpawnAreaRadius(everstead->options.renderDistance);
    everstead->m_level->SetRenderDistance(everstead->options.renderDistance);
    everstead->m_level->SetEmptyChunksSolid(true);
    everstead->m_level->SetEmptyChunksSolidForMeshing(true);

    everstead->m_camera        = new Camera();
    everstead->m_levelRenderer = new LevelRenderer(everstead->m_level, everstead->m_width, everstead->m_height);
    everstead->m_level->SetLevelRenderer(everstead->m_levelRenderer);

    everstead->m_localPlayer = new MultiPlayerLocalPlayer(everstead->m_level, L"");
    everstead->m_localPlayer->SetPosition(everstead->m_playerSpawnPosition);
    everstead->m_localPlayer->Update(1.0f);
    everstead->m_level->AddEntity(std::unique_ptr<Entity>(everstead->m_localPlayer));

    int spawnRadius     = everstead->m_worldLevel->GetSpawnAreaRadius();
    int spawnChunkCount = 0;
    for (int dz = -spawnRadius; dz <= spawnRadius; dz++)
    {
        for (int dx = -spawnRadius; dx <= spawnRadius; dx++)
        {
            if (dx * dx + dz * dz <= spawnRadius * spawnRadius)
            {
                spawnChunkCount++;
            }
        }
    }

    everstead->m_spawnChunkTarget = std::max(spawnChunkCount, 1);
    everstead->m_chunkManager->Update(everstead->m_playerSpawnPosition);
}

void EversteadWorldLoader::UpdateWorldLoading(Everstead *everstead)
{
    everstead->m_loadingTicks++;

    UIScene_WorldLoading *worldLoadingScene =
            everstead->m_uiController ? (UIScene_WorldLoading *) everstead->m_uiController->FindScene(eUIScene_WorldLoading) : nullptr;

    if (everstead->m_loadingPhase == Everstead::eLoadingPhase_FindingSeed)
    {
        if (everstead->m_loadingTicks >= 1)
        {
            if (everstead->m_joiningOnlineWorld)
            {
                if (everstead->IsSessionBanned(everstead->m_joinSessionId))
                {
                    everstead->m_loadingPhase       = Everstead::eLoadingPhase_None;
                    everstead->m_loadingOnlineWorld = false;
                    everstead->m_joiningOnlineWorld = false;
                    everstead->m_joinSessionId      = 0;
                    everstead->m_uiController->CloseScene();
                    everstead->ShowDisconnectReason(eDisconnectReason_Banned, L"");
                    return;
                }

                if (worldLoadingScene)
                {
                    worldLoadingScene->SetTitle(everstead->GetString("loading.joining_world"));
                    worldLoadingScene->SetStatus(everstead->GetString("loading.connecting_to_host"));
                    worldLoadingScene->SetProgress(1.0f);
                }

                BeginClientWorldInit(everstead);
                everstead->m_multiplayerSession->SetServerLevel(nullptr);
                if (!everstead->m_multiplayerSession->BeginJoinRoom(everstead->m_joinSessionId))
                {
                    AbortWorldLoad(everstead);
                    return;
                }

                everstead->m_loadingPhase = Everstead::eLoadingPhase_ConnectingSession;
                return;
            }

            everstead->m_worldSeed = WorldSeed::Parse(everstead->m_newWorldSeed);
            if (worldLoadingScene)
            {
                worldLoadingScene->SetTitle(everstead->GetString("loading.creating_world"));
                worldLoadingScene->SetStatus(everstead->GetString("loading.creating_level_data"));
                worldLoadingScene->SetProgress(1.0f);
            }

            BeginWorldInit(everstead);
            everstead->m_loadingPhase = Everstead::eLoadingPhase_LoadingSpawnGeneration;
            everstead->m_loadingTicks = 0;
            if (worldLoadingScene)
            {
                worldLoadingScene->SetStatus(everstead->GetString("loading.preparing_spawn_chunks"));
                worldLoadingScene->SetProgress(0.0f);
            }
        }

        return;
    }

    if (everstead->m_loadingPhase == Everstead::eLoadingPhase_LoadingSaveData)
    {
        if (worldLoadingScene)
        {
            worldLoadingScene->SetTitle(everstead->GetString("loading.loading_world"));
            worldLoadingScene->SetStatus(everstead->GetString("loading.loading_saved_region_data"));
            worldLoadingScene->SetProgress(everstead->m_loadProgress.load());
        }

        if (!everstead->m_loadFinished.load())
        {
            return;
        }

        if (everstead->m_loadThread.joinable())
        {
            everstead->m_loadThread.join();
        }

        if (!everstead->m_loadSucceeded.load())
        {
            everstead->m_worldLock.Release();
            if (everstead->m_worldLevel)
            {
                delete everstead->m_worldLevel;
                everstead->m_worldLevel = nullptr;
            }

            everstead->m_loadingPhase         = Everstead::eLoadingPhase_None;
            everstead->m_loadingExistingWorld = false;
            everstead->m_loadingOnlineWorld   = false;
            everstead->m_joiningOnlineWorld   = false;
            everstead->m_joinSessionId        = 0;
            everstead->m_worldLoadName.clear();
            everstead->m_uiController->CloseScene();
            return;
        }

        if (worldLoadingScene)
        {
            worldLoadingScene->SetStatus(everstead->GetString("loading.preparing_saved_world"));
            worldLoadingScene->SetProgress(1.0f);
        }

        BeginWorldInit(everstead);
        everstead->m_loadingPhase = Everstead::eLoadingPhase_LoadingSpawnGeneration;
        everstead->m_loadingTicks = 0;
        if (worldLoadingScene)
        {
            worldLoadingScene->SetStatus(everstead->GetString("loading.preparing_spawn_chunks"));
            worldLoadingScene->SetProgress(0.0f);
        }

        return;
    }

    if (everstead->m_loadingPhase == Everstead::eLoadingPhase_LoadingSpawnGeneration)
    {
        if (everstead->m_chunkManager)
        {
            everstead->m_chunkManager->Update(everstead->m_playerSpawnPosition);
        }

        if (everstead->m_worldLevel)
        {
            everstead->m_worldLevel->UpdateChunks(EversteadServer::GetInstance()->settings.chunkIntakePerTick);
            everstead->m_worldLevel->UpdateLighting(4000);
        }

        int spawnRadius = everstead->m_worldLevel ? everstead->m_worldLevel->GetSpawnAreaRadius() : 10;
        int cx          = Mth::FloorDiv((int) everstead->m_playerSpawnPosition.x, Chunk::s_sizeX);
        int cz          = Mth::FloorDiv((int) everstead->m_playerSpawnPosition.z, Chunk::s_sizeZ);
        int generated   = 0;
        for (int dz = -spawnRadius; dz <= spawnRadius; dz++)
        {
            for (int dx = -spawnRadius; dx <= spawnRadius; dx++)
            {
                if (dx * dx + dz * dz > spawnRadius * spawnRadius)
                {
                    continue;
                }

                ChunkPos pos(cx + dx, cz + dz);
                if (everstead->m_worldLevel && everstead->m_worldLevel->HasChunk(pos))
                {
                    generated++;
                }
            }
        }

        float generatedProgress = std::clamp((float) generated / (float) everstead->m_spawnChunkTarget, 0.0f, 1.0f);

        if (worldLoadingScene)
        {
            worldLoadingScene->SetTitle(everstead->m_loadingExistingWorld ? everstead->GetString("loading.loading_world")
                                                                          : everstead->GetString("loading.creating_world"));
            worldLoadingScene->SetStatus(everstead->GetString("loading.preparing_spawn_chunks"));
            worldLoadingScene->SetProgress(generatedProgress);
        }

        if (generated >= everstead->m_spawnChunkTarget)
        {
            everstead->m_multiplayerSession->SetServerLevel(everstead->m_worldLevel);

            bool networkReady = false;
            if (everstead->m_loadingOnlineWorld && everstead->m_joiningOnlineWorld)
            {
                networkReady = everstead->m_multiplayerSession->BeginJoinRoom(everstead->m_joinSessionId);
            }
            else
            {
                networkReady = everstead->m_multiplayerSession->BeginCreateRoom(everstead->m_worldName, everstead->m_worldLevel->GetMaxPlayers(),
                                                                                everstead->m_loadingOnlineWorld);
            }

            if (!networkReady)
            {
                AbortWorldLoad(everstead);
                return;
            }

            everstead->m_loadingPhase = Everstead::eLoadingPhase_ConnectingSession;
            UpdateSessionConnect(everstead, worldLoadingScene);
            return;
        }

        return;
    }

    if (everstead->m_loadingPhase == Everstead::eLoadingPhase_ConnectingSession)
    {
        UpdateSessionConnect(everstead, worldLoadingScene);
        return;
    }

    if (everstead->m_loadingPhase == Everstead::eLoadingPhase_WaitingForWorldPackets)
    {
        MultiPlayerLocalPlayer *multiPlayerLocalPlayer = everstead->m_localPlayer;
        ClientConnection *clientConnection             = multiPlayerLocalPlayer->GetClientConnection();

        everstead->m_timer->AdvanceTime(Time::GetDelta());
        int elapsedTicks = everstead->m_timer->GetElapsedTicks();
        for (int i = 0; i < elapsedTicks; i++)
        {
            clientConnection->Tick();
        }

        if (clientConnection->IsDisconnected())
        {
            eDISCONNECT_REASON reason = clientConnection->GetDisconnectReason();
            if (reason == eDisconnectReason_Banned)
            {
                everstead->AddBannedSession(everstead->m_multiplayerSession->GetCurrentSession().sid);
            }

            everstead->m_loadingPhase = Everstead::eLoadingPhase_None;
            everstead->FinishLeaveWorld();
            everstead->ShowDisconnectReason(reason, clientConnection->GetDisconnectMessage());
            return;
        }

        if (clientConnection->CanEnterWorld())
        {
            multiPlayerLocalPlayer->SetPlayerIndex(clientConnection->GetPlayerIndex());
            FinalizeWorldLoad(everstead);
            return;
        }

        if (worldLoadingScene)
        {
            worldLoadingScene->SetStatus(everstead->GetString("loading.joining_world_status"));
            worldLoadingScene->SetProgress(1.0f);
        }

        return;
    }
}

void EversteadWorldLoader::AbortWorldLoad(Everstead *everstead)
{
    everstead->m_loadingPhase       = Everstead::eLoadingPhase_None;
    everstead->m_loadingOnlineWorld = false;
    everstead->m_joiningOnlineWorld = false;
    everstead->m_joinSessionId      = 0;
    everstead->m_uiController->CloseScene();
}

void EversteadWorldLoader::UpdateSessionConnect(Everstead *everstead, UIScene_WorldLoading *worldLoadingScene)
{
    static const char *s_stageKeys[] = {"loading.finding_session", "loading.requesting_ticket", "loading.connecting_to_host",
                                        "loading.creating_session"};

    MultiplayerSession *session                = everstead->m_multiplayerSession;
    MultiplayerSession::eROOM_TASK_STATE state = session->PollRoomTask();
    if (state == MultiplayerSession::eRoomTask_Pending)
    {
        if (worldLoadingScene)
        {
            worldLoadingScene->SetStatus(everstead->GetString(s_stageKeys[session->GetRoomTaskStage()]));
            worldLoadingScene->SetProgress(1.0f);
        }

        return;
    }

    if (state == MultiplayerSession::eRoomTask_Failed)
    {
        // Leaving the world clears every scene, so we reopen the error the failed task already showed
        UIScene_ModalDialog::Dialog dialog = everstead->m_uiController->GetModalDialog();
        everstead->m_loadingPhase          = Everstead::eLoadingPhase_None;
        everstead->FinishLeaveWorld();
        everstead->m_uiController->ShowModal(dialog);
        return;
    }

    MultiPlayerLocalPlayer *multiPlayerLocalPlayer = everstead->m_localPlayer;
    ClientConnection *takenClientConnection        = session->TakeClientConnection();
    if (state == MultiplayerSession::eRoomTask_None || !takenClientConnection)
    {
        everstead->m_loadingPhase = Everstead::eLoadingPhase_None;
        everstead->FinishLeaveWorld();
        return;
    }

    multiPlayerLocalPlayer->SetClientConnection(takenClientConnection);
    ClientConnection *clientConnection = multiPlayerLocalPlayer->GetClientConnection();
    multiPlayerLocalPlayer->SetPlayerIndex(clientConnection->GetPlayerIndex());

    everstead->m_loadingPhase       = Everstead::eLoadingPhase_WaitingForWorldPackets;
    everstead->m_loadingOnlineWorld = false;
    everstead->m_joiningOnlineWorld = false;
    everstead->m_joinSessionId      = 0;
    if (worldLoadingScene)
    {
        worldLoadingScene->SetStatus(everstead->GetString("loading.joining_world_status"));
        worldLoadingScene->SetProgress(1.0f);
    }

    if (clientConnection->CanEnterWorld())
    {
        FinalizeWorldLoad(everstead);
    }
}

void EversteadWorldLoader::FinalizeWorldLoad(Everstead *everstead)
{
    everstead->m_uiController->ClearScenes();
    everstead->m_uiController->OpenScene(eUIScene_HUD);
    everstead->m_hudHidden = false;

    UIScene *debugOverlayScene = everstead->m_uiController->OpenScene(eUIScene_DebugOverlay);
    if (debugOverlayScene)
    {
        debugOverlayScene->SetVisible(false);
    }

    everstead->m_loadingPhase = Everstead::eLoadingPhase_None;
    everstead->m_inWorld      = true;
    if (everstead->m_chunkManager)
    {
        everstead->m_chunkManager->SetWorldName(everstead->m_worldName.empty() ? "New World" : everstead->m_worldName);
    }
    everstead->m_worldSaved           = everstead->m_loadingExistingWorld;
    everstead->m_loadingExistingWorld = false;
    everstead->m_worldLoadName.clear();
    everstead->m_inputManager->Clear();
    everstead->SetMouseLock(true);

    everstead->m_loadingOnlineWorld = false;
    everstead->m_joiningOnlineWorld = false;
    everstead->m_joinSessionId      = 0;
}

#endif
