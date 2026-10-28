#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "../entity/player/MultiPlayerLocalPlayer.h"
#include "../input/InputManager.h"
#include "../network/AccountService.h"
#include "../network/MultiplayerSession.h"
#include "../rendering/Font.h"
#include "../rendering/Framebuffer.h"
#include "../rendering/GameRenderer.h"
#include "../scene/Camera.h"
#include "../sound/MusicManager.h"
#include "../ui/Gui.h"
#include "../ui/ImGuiSystem.h"
#include "../ui/control/UIController.h"
#include "../utils/Timer.h"
#include "../world/LevelRenderer.h"
#include "../world/MultiPlayerLevel.h"
#include "../world/chunk/ChunkManager.h"
#include "../world/chunk/ChunkPos.h"
#include "../world/chunk/ChunkUnloader.h"
#include "../world/save/BackgroundSaver.h"
#include "../world/save/WorldLock.h"
#include "DiscordPresence.h"
#include "Localization.h"
#include "MemoryMonitor.h"
#include "Options.h"
#include "TickRateManager.h"
#include "Window.h"

class EversteadWorldLoader;
class SoundEngine;
class ResourceLoader;
class SaveService;
class ServerLevel;

enum eDISCONNECT_REASON : uint8_t;

class Everstead
{
public:
    static Everstead *GetInstance();

    void Tick();
    void Render();
    void RequestClose();
    bool ShouldClose() const;
    void Shutdown();

    int GetWidth() const;
    int GetHeight() const;
    bool IsWorldMenuOpen() const;

    void SaveOptions();
    void SetVSync(bool enabled);
    void SetWindowMode(int mode);
    void ApplyDisplayOptions();
    void ApplyTextureOptions();
    void ApplyMeshingOptions();
    void SetDiscordRichPresence(bool enabled);
    void UpdateDiscordPresence();
    eMUSIC_TYPE GetMusicType() const;
    int GetChunkWorkerThreadsPerPool() const;
    void SetRenderDistance(int distance);
    void SetSimulationDistance(int distance);

    void StartWorldLoad();
    void StartNewWorld(const std::string &worldName, const std::string &seed, int difficulty, int gameType);
    void StartJoinedWorld(uint64_t sid, const std::string &worldName);
    void StartSavedWorld(const std::string &worldName, bool online = false);
    void OpenLoadWorldMenu(const std::string &worldName);
    const std::string &GetSelectedWorldName() const;
    uint32_t GetSelectedWorldSeed() const;
    GameRules &GetNewWorldGameRules();
    WorldGenSettings &GetNewWorldGenSettings();
    bool IsWorldLoading() const;
    void OpenPauseMenu();
    void ClosePauseMenu();
    void OpenInventory(eUISCENE sceneId);
    void CloseInventory();
    bool IsInventoryOpen() const;
    void RequestLeaveWorld(bool save);
    void JoinAfterLeavingWorld(uint64_t sid, const std::string &worldName);
    void LeaveWorld();
    void FinishLeaveWorld();
    void ShowDisconnectReason(eDISCONNECT_REASON reason, const std::wstring &text);
    void AddBannedSession(uint64_t sid);
    bool IsSessionBanned(uint64_t sid) const;
    void SaveWorld();
    bool CanSaveWorld() const;
    bool IsSaving() const;
    int GetTicksUntilAutosave() const;
    void ResetAutosaveTimer();

    Timer *GetTimer() const;
    TickRateManager *GetTickRateManager();
    void SetTickingState(float tickRate, bool frozen);
    const std::array<int, 4> &GetStickFightScores() const;
    void SetStickFightScores(const std::array<int, 4> &scores);
    float GetRenderPartialTicks() const;

    const Camera *GetCamera() const;

    MultiPlayerLocalPlayer *GetLocalPlayer() const;
    void ToggleNimbusMenu();
    MultiPlayerLevel *GetLevel() const;
    Level *GetWorldLevel() const;

    LevelRenderer *GetLevelRenderer() const;

    ChunkManager *GetChunkManager() const;

    Font *GetDefaultFont() const;
    InputManager *GetInputManager() const;
    AccountService *GetAccountService() const;
    MultiplayerSession *GetMultiplayerSession() const;
    UIController *GetUiController() const;
    Gui *GetGui() const;
    bool IsHudVisible() const;
    void StartDimensionTransition();
    void FinishDimensionTransition();
    GameRenderer *GetGameRenderer() const;
    Framebuffer *GetFrameFramebuffer() const;
    SoundEngine *GetSoundEngine() const;
    MusicManager *GetMusicManager();
    std::wstring GetString(const std::string &key) const;

    Options options;

private:
    static constexpr float kPanoramaGamma = 50.0f;

    friend class EversteadWorldLoader;
    friend class LocalPlayer;

    Everstead();
    ~Everstead();

    Everstead(const Everstead &)            = delete;
    Everstead &operator=(const Everstead &) = delete;

    Everstead(Everstead &&)            = delete;
    Everstead &operator=(Everstead &&) = delete;

    void RenderFrame();
    void SetMouseLock(bool locked);
    void ToggleDebugOverlay();
    void ToggleDebugOverlay(bool withProfiler);
    void ToggleHud();
    void AddDebugMessage(const std::string &translationKey);

    void UpdateResourceLoading();
    void SyncRenderSizeFromWindow();
    void ApplyRenderSize(int width, int height);

    void TakeScreenshot();
    void TakePanorama();
    void CaptureWorldIcon(const std::string &worldName);

    void UpdateSaving();
    void BackgroundSave();
    void SetServerAttached(bool attached);
    void RunServerThread();
    void TickServerThread();
    void ApplyServerSettings();
    std::string GetSaveWorldName() const;
    void ReportChunkGenerationFailure(const ChunkPos &pos);
    void OpenSavingScene(const std::wstring &status, float progress);
    void CloseSavingScene();
    void OpenLeaveWorldLoadingScene();
    void BeginWorldInit();
    void UpdateWorldLoading();
    void FinalizeWorldLoad();

    int m_width;
    int m_height;
    Window m_window;
    DiscordPresence m_discordPresence;
    int64_t m_presenceStartSeconds;
    uint32_t m_lastPresenceUpdateMs;
    bool m_presenceInWorld;
    static const uint32_t s_presenceUpdateIntervalMs = 1000;

    bool m_shutdown;

    Timer *m_timer;
    TickRateManager m_tickRateManager;
    std::array<int, 4> m_stickFightScores;
    float m_renderPartialTicks = 0.0f;

    Camera *m_camera;
    MultiPlayerLevel *m_level;
    ServerLevel *m_worldLevel;

    MultiPlayerLocalPlayer *m_localPlayer;

    LevelRenderer *m_levelRenderer;
    ChunkManager *m_chunkManager;

    Font *m_defaultFont;
    Framebuffer *m_frameFramebuffer;
    GameRenderer *m_gameRenderer;
    UIController *m_uiController;
    Gui *m_gui;
    ImGuiSystem *m_imguiSystem;
    InputManager *m_inputManager;
    SoundEngine *m_soundEngine;
    MusicManager m_musicManager;
    AccountService *m_accountService;
    MultiplayerSession *m_multiplayerSession;

    glm::dvec3 m_playerSpawnPosition;

    bool m_inWorld;
    bool m_mouseLocked;
    bool m_restoreMouseLockAfterBlockingUi;
    bool m_leaveWorldRequested;
    bool m_leaveWorldScreenVisible;
    bool m_hudHidden;
    bool m_leavingWorld;
    bool m_leaveWorldSave;
    bool m_worldSaved;

    enum eLOADING_PHASE : uint8_t
    {
        eLoadingPhase_None                   = 0,
        eLoadingPhase_FindingSeed            = 1,
        eLoadingPhase_LoadingSaveData        = 2,
        eLoadingPhase_LoadingSpawnGeneration = 3,
        eLoadingPhase_WaitingForWorldPackets = 4,
        eLoadingPhase_ConnectingSession      = 5,
    };

    eLOADING_PHASE m_loadingPhase;
    int m_spawnChunkTarget;
    int m_loadingTicks;
    uint32_t m_worldSeed;
    int m_newWorldDifficulty;
    int m_newWorldGameType;
    std::string m_newWorldSeed;
    GameRules m_newWorldGameRules;
    WorldGenSettings m_newWorldGenSettings;
    bool m_loadingExistingWorld;
    bool m_loadingOnlineWorld;
    bool m_joiningOnlineWorld;
    uint64_t m_joinSessionId;
    uint64_t m_pendingJoinSid;
    std::string m_pendingJoinWorldName;
    std::string m_worldName;
    std::string m_worldLoadName;
    std::string m_selectedWorldName;
    uint32_t m_selectedWorldSeed;

    std::thread m_loadThread;
    std::atomic<bool> m_loadFinished;
    std::atomic<bool> m_loadSucceeded;
    std::atomic<float> m_loadProgress;

    bool m_captureWorldIcon;
    ResourceLoader *m_resourceLoader;
    bool m_takeScreenshot;
    bool m_takePanorama;
    std::string m_iconWorldName;

    std::thread m_saveThread;
    std::atomic<bool> m_saving;
    std::atomic<bool> m_saveFinished;
    std::atomic<float> m_saveProgress;
    SaveService *m_saveService;
    BackgroundSaver m_backgroundSaver;
    WorldLock m_worldLock;
    ChunkUnloader m_chunkUnloader;
    std::atomic<int> m_ticksSinceAutosave;
    bool m_backgroundSaveActive;
    uint32_t m_backgroundSaveStartMs;
    std::thread m_serverThread;
    std::condition_variable m_serverCondition;
    std::atomic<bool> m_serverThreadRunning;
    std::atomic<bool> m_serverAttached;
    std::atomic<bool> m_serverSimulating;
    std::atomic<bool> m_saveRequested;
    std::vector<uint64_t> m_bannedSessions;
};
