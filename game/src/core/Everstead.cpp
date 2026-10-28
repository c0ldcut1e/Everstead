#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Everstead.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include <SDL2/SDL.h>

#include "../entity/player/MultiPlayerLocalPlayer.h"
#include "../entity/player/ServerPlayer.h"
#include "../input/SdlControllerBackend.h"
#include "../io/FileSystem.h"
#include "../network/PlayerList.h"
#include "../network/listener/ClientConnection.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/s2c/ChatS2CPacket.h"
#include "../network/packet/s2c/DisconnectS2CPacket.h"
#include "../platform/PlatformThread.h"
#include "../rendering/BlockAtlas.h"
#include "../rendering/Framebuffer.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/ItemAtlas.h"
#include "../rendering/PngEncoder.h"
#include "../rendering/RenderBackend.h"
#include "../rendering/RenderDevice.h"
#include "../rendering/Tesselator.h"
#include "../rendering/TextureAtlas.h"
#include "../rendering/TextureRepository.h"
#include "../sound/SoundEngine.h"
#include "../sound/SoundRegistry.h"
#include "../tag/CompoundTag.h"
#include "../ui/ImGuiSystem.h"
#include "../ui/Screen.h"
#include "../ui/scene/UIScene_DebugOverlay.h"
#include "../ui/scene/UIScene_HUD.h"
#include "../ui/scene/UIScene_MainMenu.h"
#include "../ui/scene/UIScene_ModalDialog.h"
#include "../ui/scene/UIScene_WorldLoading.h"
#include "../ui/scene/UIScene_WorldSaving.h"
#include "../utils/StringUtils.h"
#include "../utils/Time.h"
#include "../utils/math/Mth.h"
#include "../world/LevelRenderer.h"
#include "../world/ServerLevel.h"
#include "../world/biome/Biome.h"
#include "../world/block/Block.h"
#include "../world/block/BlockRegistry.h"
#include "../world/chunk/Chunk.h"
#include "../world/chunk/ChunkMesher.h"
#include "../world/chunk/ChunkPos.h"
#include "../world/inventory/Container.h"
#include "../world/inventory/ServerContainerMenu.h"
#include "../world/lighting/LightSource.h"
#include "../world/lod/LodGenerator.h"
#include "../world/save/SaveService.h"
#include "../world/save/WorldSave.h"
#include "EversteadServer.h"
#include "EversteadWorldLoader.h"
#include "FrameProfiler.h"
#include "GameInfo.h"
#include "Logger.h"
#include "ResourceLoader.h"
#include "Screenshot.h"
#include "events/EventDispatcher.h"
#include "events/EventManager.h"
#include "events/KeyPressedEvent.h"
#include "events/KeyReleasedEvent.h"
#include "events/MouseButtonPressedEvent.h"
#include "events/MouseButtonReleasedEvent.h"
#include "events/MouseMovedEvent.h"
#include "events/MouseWheelEvent.h"
#include "events/SdlEventBridge.h"
#include "events/TextInputEvent.h"
#include "events/WindowCloseEvent.h"
#include "events/WindowResizedEvent.h"
#include "ui/UIContainerInput.h"
#include "world/inventory/AbstractContainer.h"

#include <glm/gtx/norm.hpp>

static Everstead *s_eversteadInstance = nullptr;

Everstead::Everstead()
{
    s_eversteadInstance = this;

    m_width  = 1280;
    m_height = 720;
    options.Reset();
    options.Load();
    m_window.Create(m_width, m_height, "Everstead", RenderBackend::kBackends[RenderBackend::FindIndex(options.renderBackend)].backend);
    m_shutdown                        = false;
    m_timer                           = nullptr;
    m_camera                          = nullptr;
    m_level                           = nullptr;
    m_worldLevel                      = nullptr;
    m_localPlayer                     = nullptr;
    m_levelRenderer                   = nullptr;
    m_chunkManager                    = nullptr;
    m_defaultFont                     = nullptr;
    m_frameFramebuffer                = nullptr;
    m_gameRenderer                    = nullptr;
    m_uiController                    = nullptr;
    m_gui                             = nullptr;
    m_imguiSystem                     = nullptr;
    m_inputManager                    = nullptr;
    m_soundEngine                     = nullptr;
    m_accountService                  = nullptr;
    m_multiplayerSession              = nullptr;
    m_playerSpawnPosition             = {};
    m_inWorld                         = false;
    m_mouseLocked                     = false;
    m_restoreMouseLockAfterBlockingUi = false;
    m_leaveWorldRequested             = false;
    m_leaveWorldScreenVisible         = false;
    m_hudHidden                       = false;
    m_leavingWorld                    = false;
    m_leaveWorldSave                  = true;
    m_pendingJoinSid                  = 0;
    m_worldSaved                      = false;
    m_stickFightScores.fill(-1);
    m_loadingPhase         = eLoadingPhase_None;
    m_spawnChunkTarget     = 0;
    m_loadingTicks         = 0;
    m_worldSeed            = 0;
    m_newWorldDifficulty   = 2;
    m_newWorldGameType     = 0;
    m_loadingExistingWorld = false;
    m_loadingOnlineWorld   = false;
    m_joiningOnlineWorld   = false;
    m_joinSessionId        = 0;
    m_worldName            = {};
    m_worldLoadName        = {};
    m_selectedWorldName    = {};
    m_selectedWorldSeed    = 0;
    m_loadFinished         = false;
    m_loadSucceeded        = false;
    m_loadProgress         = 0.0f;
    m_saving               = false;
    m_saveFinished         = false;
    m_saveProgress         = 0.0f;
    m_saveService          = new SaveService();
    m_saveService->Start();
    m_ticksSinceAutosave  = 0;
    m_serverThreadRunning = false;
    m_serverAttached      = false;
    m_serverSimulating    = false;
    m_saveRequested       = false;

    MemoryMonitor::GetInstance()->ApplyLimit(MemoryMonitor::s_defaultMemoryLimitMb);

    const char *profileIntervalEnv = getenv("EVERSTEAD_PROFILE_INTERVAL");
    if (profileIntervalEnv)
    {
        double intervalMs = atof(profileIntervalEnv);
        if (intervalMs > 0.0)
        {
            FrameProfiler::SetAverageWindowMs(intervalMs);
        }
    }

    const char *profileHoldEnv = getenv("EVERSTEAD_PROFILE_HOLD");
    if (profileHoldEnv)
    {
        double holdMs = atof(profileHoldEnv);
        if (holdMs > 0.0)
        {
            FrameProfiler::SetWorstHoldMs(holdMs);
        }
    }

    const char *profileEnv = getenv("EVERSTEAD_PROFILE");
    if (profileEnv)
    {
        double thresholdMs = atof(profileEnv);
        if (thresholdMs > 0.0)
        {
            FrameProfiler::SetThresholdMs(thresholdMs);
        }

        FrameProfiler::SetLogSlowFrames(true);
        FrameProfiler::SetEnabled(true);
        Logger::LogInfo("Frame profiler logging enabled, threshold %.2fms", FrameProfiler::GetThresholdMs());
    }

    m_backgroundSaveActive  = false;
    m_backgroundSaveStartMs = 0;

    m_captureWorldIcon = false;
    m_resourceLoader   = nullptr;
    m_takeScreenshot   = false;
    m_takePanorama     = false;
    m_iconWorldName    = "";
    m_bannedSessions   = {};
    Logger::LogInfo("Render device initialized: %s", RenderDevice::GetInstance()->GetVersion());

    InitSdlEventBridge(m_window.GetHandle());
    Logger::LogInfo("Event bridge initialized");

    m_accountService     = new AccountService();
    m_multiplayerSession = new MultiplayerSession();
    m_multiplayerSession->SetAccountService(m_accountService);

    m_imguiSystem = new ImGuiSystem();
    m_imguiSystem->Init(m_window.GetHandle());

    RenderDevice::GetInstance()->SetVSync(options.vsync);
    m_window.SetMode((eWINDOW_MODE) options.windowMode);
    m_presenceStartSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_lastPresenceUpdateMs = 0;
    m_presenceInWorld      = false;
    SetDiscordRichPresence(options.discordRichPresence);

    m_timer        = new Timer(20.0f, 0.25f);
    m_inputManager = new InputManager(std::make_unique<SdlControllerBackend>());
    for (size_t i = 0; i < g_keyBindingCount; i++)
    {
        m_inputManager->SetBinding((eKEY_BINDING) i, options.keyBindings[i]);
    }

    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        m_inputManager->SetControllerBinding((eCONTROLLER_BINDING) i, (eCONTROLLER_BUTTON) options.controllerBindings[i]);
    }

    m_inputManager->SetControllerLookSensitivity(options.controllerSensitivity);
    m_inputManager->SetToggleOptions(options.keyboardToggleSprint, options.keyboardToggleSneak, options.controllerToggleSprint,
                                     options.controllerToggleSneak);

    m_defaultFont      = new Font("assets/font/Default.png", 24);
    m_frameFramebuffer = new Framebuffer(m_width, m_height);
    m_gameRenderer     = new GameRenderer();
    m_gameRenderer->SetFov(options.fov);
    m_gameRenderer->Init();
    m_gameRenderer->SetGamma(options.gamma);
    m_gameRenderer->UpdateProjection(m_width, m_height);

    Localization *localization = Localization::GetInstance();
    localization->ReloadLanguages();
    if (!localization->LoadLanguage("en_us") && !localization->GetLanguages().empty())
    {
        localization->LoadLanguage(localization->GetLanguages()[0].code);
    }

    m_gui          = new Gui();
    m_uiController = new UIController();
    ((UIScene_WorldLoading *) m_uiController->OpenScene(eUIScene_WorldLoading))->SetTitle(GetString("loading.game"));

    m_soundEngine = new SoundEngine();
    SoundRegistry::Init();
    m_soundEngine->Init(&options);

    EventManager::AddListener(
            [this](Event &event)
            {
                EventDispatcher dispatcher(event);

                dispatcher.Dispatch<KeyPressedEvent>(
                        [this](KeyPressedEvent &event)
                        {
                            if (m_uiController->IsModalActive())
                            {
                                m_inputManager->OnKeyPressed(event.GetKey());

                                return true;
                            }

                            if (m_imguiSystem->WantsKeyboardCapture())
                            {
                                return true;
                            }

                            m_inputManager->OnKeyPressed(event.GetKey());
                            return true;
                        });

                dispatcher.Dispatch<KeyReleasedEvent>(
                        [this](KeyReleasedEvent &event)
                        {
                            if (m_uiController->IsModalActive())
                            {
                                m_inputManager->OnKeyReleased(event.GetKey());

                                return true;
                            }

                            if (m_imguiSystem->WantsKeyboardCapture())
                            {
                                return true;
                            }

                            m_inputManager->OnKeyReleased(event.GetKey());

                            return true;
                        });

                dispatcher.Dispatch<TextInputEvent>(
                        [this](TextInputEvent &event)
                        {
                            if (m_uiController->IsModalActive())
                            {
                                return true;
                            }

                            if (m_imguiSystem->WantsKeyboardCapture())
                            {
                                return true;
                            }

                            m_inputManager->OnTextInput(event.GetText());

                            return true;
                        });

                dispatcher.Dispatch<MouseButtonPressedEvent>(
                        [this](MouseButtonPressedEvent &event)
                        {
                            if (m_uiController->IsModalActive())
                            {
                                return true;
                            }

                            if (m_imguiSystem->WantsMouseCapture())
                            {
                                return true;
                            }

                            m_inputManager->OnMouseButtonPressed(event.GetButton());

                            return true;
                        });

                dispatcher.Dispatch<MouseButtonReleasedEvent>(
                        [this](MouseButtonReleasedEvent &event)
                        {
                            m_inputManager->OnMouseButtonReleased(event.GetButton());

                            return true;
                        });

                dispatcher.Dispatch<MouseMovedEvent>(
                        [this](MouseMovedEvent &event)
                        {
                            m_inputManager->OnMouseMoved(event.GetDeltaX(), event.GetDeltaY(),
                                                         !m_uiController->IsModalActive() && m_mouseLocked && !m_imguiSystem->WantsMouseCapture());

                            return true;
                        });

                dispatcher.Dispatch<MouseWheelEvent>(
                        [this](MouseWheelEvent &event)
                        {
                            m_inputManager->OnMouseWheel(event.GetWheelY());

                            return true;
                        });

                dispatcher.Dispatch<WindowResizedEvent>(
                        [this](WindowResizedEvent &event)
                        {
                            ApplyRenderSize(event.GetWidth(), event.GetHeight());

                            return true;
                        });

                dispatcher.Dispatch<WindowCloseEvent>(
                        [this](WindowCloseEvent &)
                        {
                            RequestClose();
                            return true;
                        });
            });

    SetMouseLock(false);

    ChunkMesher::s_ambientOcclusionEnabled = options.ambientOcclusion;
    ChunkMesher::s_smartLeavesEnabled      = options.smartLeaves;
    SyncRenderSizeFromWindow();
    m_resourceLoader = new ResourceLoader();
}

Everstead::~Everstead()
{
    Shutdown();
}

Everstead *Everstead::GetInstance()
{
    if (s_eversteadInstance)
    {
        return s_eversteadInstance;
    }

    static Everstead instance;
    s_eversteadInstance = &instance;
    return &instance;
}

void Everstead::Tick()
{
    if (m_shutdown)
    {
        return;
    }

    FrameProfilerScope tickScope("Everstead::Tick");
    UpdateDiscordPresence();

    {
        FrameProfilerScope scope("SdlEvents");
        PumpSdlEvents();
    }

    {
        FrameProfilerScope scope("EventManager");
        EventManager::Process();
    }

    {
        FrameProfilerScope scope("SoundEngine::Tick");
        m_soundEngine->Tick((float) Time::GetDelta());
        m_musicManager.Update(m_soundEngine, GetMusicType(), (float) Time::GetDelta());
    }

    {
        FrameProfilerScope scope("Input::Update");
        m_inputManager->Update(Time::GetDelta());
    }

    SDL_ShowCursor(!m_mouseLocked && !m_inputManager->IsUsingController() ? SDL_ENABLE : SDL_DISABLE);

    MemoryMonitor::GetInstance()->Tick();

    if (m_resourceLoader)
    {
        UpdateResourceLoading();
        m_uiController->Tick();
        return;
    }

    bool debugKeysBlocked = IsWorldMenuOpen() || m_gui->IsChatOpen() || m_uiController->IsModalActive() || kStickFight;
    if (m_inputManager->ConsumeActionPress(eGameplayAction_ToggleProfiler) && !debugKeysBlocked)
    {
        ToggleDebugOverlay(true);
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_ToggleDebugOverlay) && !debugKeysBlocked)
    {
        ToggleDebugOverlay();
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_ToggleHitboxes) && !debugKeysBlocked && m_levelRenderer)
    {
        AddDebugMessage(m_levelRenderer->ToggleHitboxes() ? "debug.show_hitboxes.on" : "debug.show_hitboxes.off");
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_ToggleChunkBorders) && !debugKeysBlocked && m_levelRenderer)
    {
        AddDebugMessage(m_levelRenderer->ToggleChunkGrid() ? "debug.chunk_boundaries.on" : "debug.chunk_boundaries.off");
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_ReloadChunks) && !debugKeysBlocked && m_levelRenderer && m_level)
    {
        std::vector<std::pair<ChunkPos, Chunk *>> chunks = m_level->GetLoadedChunks();
        for (size_t i = 0; i < chunks.size(); i++)
        {
            m_levelRenderer->RebuildChunk(chunks[i].first);
        }

        AddDebugMessage("debug.reload_chunks.message");
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_ToggleAdvancedTooltips) && !debugKeysBlocked)
    {
        options.advancedTooltips = !options.advancedTooltips;
        options.Save();
        AddDebugMessage(options.advancedTooltips ? "debug.advanced_tooltips.on" : "debug.advanced_tooltips.off");
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_ToggleHud) && !debugKeysBlocked)
    {
        ToggleHud();
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_Screenshot))
    {
        m_takeScreenshot = true;
    }

    if (m_inputManager->ConsumeActionPress(eGameplayAction_Panorama) && m_inWorld)
    {
        m_takePanorama = true;
    }

    {
        FrameProfilerScope scope("AccountService::Tick");
        m_accountService->Tick();
    }

    {
        FrameProfilerScope scope("MultiplayerSession::Tick");
        m_multiplayerSession->Tick();
    }

    bool modalActive          = m_uiController->IsModalActive();
    bool invitationMenuActive = m_uiController->IsInvitationMenuInputBlocking();
    bool openChat             = m_inputManager->ConsumeActionPress(eGameplayAction_Chat);
    bool openCommand          = m_inputManager->ConsumeActionPress(eGameplayAction_Command);
    bool sleepMenuOpen        = m_uiController->GetTopScene() && m_uiController->GetTopScene()->GetId() == eUIScene_SleepMenu;
    if ((openChat || openCommand) && m_inWorld && !modalActive && !invitationMenuActive && (!IsWorldMenuOpen() || sleepMenuOpen) && !m_saving.load())
    {
        if (m_mouseLocked)
        {
            m_restoreMouseLockAfterBlockingUi = true;
        }

        m_gui->OpenChat(openCommand ? L"/" : L"");
        SetMouseLock(false);
    }

    if (m_gui->IsChatOpen())
    {
        m_gui->TickInput(m_inputManager);
    }

    m_uiController->TickInput(m_inputManager, m_inWorld);

    if (m_leavingWorld)
    {
        UIScene *leaveScene = m_uiController->FindScene(eUIScene_WorldLoading);
        float progress      = m_saveProgress.load();
        bool finished       = m_saveFinished.load();
        if (leaveScene)
        {
            const char *statusKey = "loading.saving_chunks";
            if (finished)
            {
                statusKey = "loading.unloading_world";
            }
            else if (progress >= 0.9f)
            {
                statusKey = "loading.saving_entities";
            }

            ((UIScene_WorldLoading *) leaveScene)->SetStatus(GetString(statusKey));
            ((UIScene_WorldLoading *) leaveScene)->SetProgress(progress);
        }

        // One frame shows the unloading status before the teardown runs
        if (finished && m_leaveWorldScreenVisible)
        {
            FinishLeaveWorld();
        }
        else if (finished)
        {
            m_leaveWorldScreenVisible = true;
        }

        m_uiController->Tick();
        return;
    }

    if (m_leaveWorldRequested)
    {
        OpenLeaveWorldLoadingScene();
        if (m_leaveWorldScreenVisible)
        {
            m_leaveWorldRequested     = false;
            m_leaveWorldScreenVisible = false;
            LeaveWorld();
            return;
        }

        m_leaveWorldScreenVisible = true;
        m_uiController->Tick();
        return;
    }

    bool chatOpen     = m_gui->IsChatOpen();
    UIScene *topScene = m_uiController->GetTopScene();
    if (topScene && (topScene->GetId() == eUIScene_HUD || topScene->GetId() == eUIScene_DebugOverlay))
    {
        m_uiController->FindScene(eUIScene_HUD)->SetVisible(!m_hudHidden || chatOpen);
    }

    bool worldMenuOpen    = IsWorldMenuOpen();
    bool blockingUiActive = modalActive || invitationMenuActive || chatOpen;
    if (blockingUiActive && m_mouseLocked)
    {
        m_restoreMouseLockAfterBlockingUi = true;
        SetMouseLock(false);
    }
    else if (!blockingUiActive && m_restoreMouseLockAfterBlockingUi && m_inWorld && !worldMenuOpen)
    {
        m_restoreMouseLockAfterBlockingUi = false;
        SetMouseLock(true);
    }
    else if (!blockingUiActive)
    {
        m_restoreMouseLockAfterBlockingUi = false;
    }

    if ((invitationMenuActive || chatOpen) && m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    if (modalActive)
    {
        if (m_localPlayer)
        {
            m_localPlayer->ClearInputs();
        }
    }

    bool online               = m_multiplayerSession->IsOnline();
    bool gameplayInputBlocked = modalActive || invitationMenuActive || chatOpen || worldMenuOpen;
    m_inputManager->SetGameplayInputBlocked(gameplayInputBlocked);
    bool worldTickBlocked =
            modalActive || invitationMenuActive ||
            (worldMenuOpen && topScene->GetId() != eUIScene_Inventory && topScene->GetId() != eUIScene_Container &&
             topScene->GetId() != eUIScene_CraftingTable && topScene->GetId() != eUIScene_RecipeView && topScene->GetId() != eUIScene_Furnace &&
             topScene->GetId() != eUIScene_ResonantSocket && topScene->GetId() != eUIScene_Resonator && topScene->GetId() != eUIScene_Anvil &&
             topScene->GetId() != eUIScene_DeathMenu && topScene->GetId() != eUIScene_SleepMenu && topScene->GetId() != eUIScene_SignEdit);
    bool worldActive = m_inWorld && !m_saving.load() && !m_leavingWorld && (online || !worldTickBlocked);

    if (ShouldClose())
    {
        return;
    }

    if (m_loadingPhase != eLoadingPhase_None)
    {
        UpdateWorldLoading();
        return;
    }

    if (m_inWorld && !gameplayInputBlocked && !m_saving.load() && !m_imguiSystem->IsVisible() && !m_mouseLocked)
    {
        SetMouseLock(true);
    }

    if (m_inWorld && !m_saving.load() && !modalActive && !invitationMenuActive && !chatOpen && !IsWorldMenuOpen() && m_localPlayer)
    {
        double lookDeltaX = 0.0;
        double lookDeltaY = 0.0;
        m_inputManager->ConsumeLookDelta(&lookDeltaX, &lookDeltaY);
        if (options.invertLook)
        {
            lookDeltaY = -lookDeltaY;
        }

        if (lookDeltaX != 0.0 || lookDeltaY != 0.0)
        {
            m_localPlayer->ApplyLookInput(lookDeltaX, lookDeltaY);
        }

        if (m_inputManager->ConsumeActionPress(eGameplayAction_TogglePerspective))
        {
            m_localPlayer->CycleThirdPersonView();
        }

        bool attackPressed = m_inputManager->ConsumeActionPress(eGameplayAction_BreakBlock);
        bool usePressed    = m_inputManager->ConsumeActionPress(eGameplayAction_PlaceBlock);
        m_localPlayer->SetClickInputs(attackPressed, m_inputManager->IsActionDown(eGameplayAction_BreakBlock), m_inputManager->IsDestroyHeld(),
                                      usePressed, m_inputManager->IsActionDown(eGameplayAction_PlaceBlock), m_inputManager->IsUseItemHeld());

        if (m_inputManager->ConsumeActionPress(eGameplayAction_PickBlock))
        {
            m_localPlayer->PickTargetedBlock((SDL_GetModState() & KMOD_CTRL) != 0);
        }

        if (m_inputManager->ConsumeActionPress(eGameplayAction_DropStack))
        {
            m_localPlayer->DropSelectedItem(true);
        }

        if (m_inputManager->ConsumeActionPress(eGameplayAction_DropItem))
        {
            m_localPlayer->DropSelectedItem(false);
        }

        if (!m_gameRenderer->GetDimensionTransition()->IsTraveling())
        {
            m_localPlayer->SetMoveInputs(m_inputManager->GetMoveForward(), m_inputManager->GetMoveStrafe());
            m_localPlayer->SetJumpHeld(m_inputManager->IsActionDown(eGameplayAction_Jump));
        }

        Inventory *inventory = m_localPlayer->GetInventory();
        int hotbarScroll     = m_inputManager->ConsumeHotbarScroll();
        bool spectator       = GameType::IsSpectator(m_localPlayer->GetGameType());
        if (hotbarScroll != 0 && spectator)
        {
            m_localPlayer->ScrollFlySpeed(hotbarScroll);
        }
        else if (hotbarScroll != 0)
        {
            inventory->ScrollSelectedSlot(-hotbarScroll);
        }

        int hotbarSlot = m_inputManager->ConsumeHotbarSlot();
        if (hotbarSlot >= 0 && !spectator)
        {
            inventory->SetSelectedSlot(hotbarSlot);
        }
    }
    else
    {
        // Drops pressed while a menu is open must not fire once it closes
        m_inputManager->ConsumeActionPress(eGameplayAction_DropStack);
        m_inputManager->ConsumeActionPress(eGameplayAction_DropItem);
        if (m_localPlayer)
        {
            m_localPlayer->SetClickInputs(false, false, false, false, false, false);
        }
    }

    m_timer->AdvanceTime(Time::GetDelta());
    int elapsedTicks = m_timer->GetElapsedTicks();
    // A paused world keeps its last partial tick so interpolation between the frozen ticks stops looping
    if (worldActive)
    {
        m_renderPartialTicks = m_timer->GetPartialTicks();
    }

    for (int i = 0; i < elapsedTicks; i++)
    {
        m_gameRenderer->TickLightFlicker();
        if (m_inWorld && m_level && m_localPlayer)
        {
            LightSource::TickDarkAdaptation(m_level, m_localPlayer->GetEyePosition(1.0f), m_levelRenderer->GetItemLights());
        }

        if (m_inWorld && m_localPlayer)
        {
            if (gameplayInputBlocked || m_saving.load() || m_gameRenderer->GetDimensionTransition()->IsTraveling())
            {
                m_localPlayer->SetMoveInputs(0.0f, 0.0f);
                m_localPlayer->SetJumpHeld(false);
                m_localPlayer->SetSprintHeld(false);
                m_localPlayer->SetSneakHeld(m_inputManager->IsSneakToggled());
                m_inputManager->ConsumeForwardTapCount();
            }
            else
            {
                float moveForward = 0.0f;
                float moveStrafe  = 0.0f;
                bool jumpHeld     = false;
                bool jumpPressed  = false;
                m_inputManager->ConsumeLocalPlayerTickInputs(&moveForward, &moveStrafe, &jumpHeld, &jumpPressed);
                m_localPlayer->SetMoveInputs(moveForward, moveStrafe);
                m_localPlayer->SetJumpHeld(jumpHeld);
                if (jumpPressed)
                {
                    m_localPlayer->RegisterJumpPress();
                }
                m_localPlayer->SetSprintHeld(m_inputManager->IsActionDown(eGameplayAction_Sprint));
                bool sneakHeld = m_localPlayer->IsFlying() ? m_inputManager->IsSneakHeld() : m_inputManager->IsActionDown(eGameplayAction_Sneak);
                if (m_localPlayer->IsFlying())
                {
                    // Descending while flying must not leave the controller sneak toggle on after landing
                    m_inputManager->ClearSneakToggle();
                }

                m_localPlayer->SetSneakHeld(sneakHeld);
                m_localPlayer->SetForwardTaps(m_inputManager->ConsumeForwardTapCount());
            }
        }

        if (worldActive && m_level)
        {
            FrameProfilerScope scope("Level::Tick");
            m_tickRateManager.Tick();
            m_level->Tick();
        }

        if (worldActive && m_levelRenderer && m_localPlayer)
        {
            m_levelRenderer->TickItemInHand(m_localPlayer);
        }
    }

    if (m_saving.load() && m_inWorld && m_localPlayer)
    {
        ClientConnection *clientConnection = m_localPlayer->GetClientConnection();
        if (clientConnection)
        {
            clientConnection->ResetTimeout();
            clientConnection->Tick();
        }
    }

    m_serverSimulating.store(worldActive);
    SetServerAttached(m_inWorld && !m_leavingWorld && m_chunkManager && EversteadServer::GetInstance()->IsRunning());

    ChunkPos failedChunk;
    if (m_chunkManager && m_chunkManager->TakeGenerationFailure(&failedChunk))
    {
        ReportChunkGenerationFailure(failedChunk);
    }

    if (worldActive && m_level)
    {
        FrameProfilerScope scope("Level::Update");
        m_level->Update(m_renderPartialTicks);
    }

    if (worldActive && m_localPlayer && m_camera)
    {
        m_gameRenderer->UpdateCamera(m_camera, m_localPlayer, m_renderPartialTicks);
    }

    if (m_inWorld && m_camera)
    {
        m_soundEngine->SetListenerPosition(m_camera->GetPosition());
        m_soundEngine->SetListenerOrientation(m_camera->GetFront(), glm::dvec3(0.0, 1.0, 0.0));
    }

    if (worldActive && m_levelRenderer)
    {
        {
            FrameProfilerScope scope("TickMeshing");
            m_levelRenderer->TickMeshing();
        }

        {
            FrameProfilerScope scope("UnloadFarClientChunks");
            m_chunkUnloader.UnloadClientChunks(m_level, m_localPlayer->GetPosition(), m_level->GetRenderDistance() + options.chunkUnloadMargin);
        }
    }

    if (m_inWorld && m_localPlayer)
    {
        ClientConnection *clientConnection = m_localPlayer->GetClientConnection();
        if (clientConnection && clientConnection->IsDisconnected())
        {
            eDISCONNECT_REASON reason = clientConnection->GetDisconnectReason();
            if (reason == eDisconnectReason_Banned)
            {
                AddBannedSession(m_multiplayerSession->GetCurrentSession().sid);
            }

            LeaveWorld();
            ShowDisconnectReason(reason, clientConnection->GetDisconnectMessage());
            return;
        }
    }

    {
        FrameProfilerScope scope("UpdateSaving");
        UpdateSaving();
    }

    {
        FrameProfilerScope scope("UIController::Tick");
        m_uiController->Tick();
    }
}

void Everstead::Render()
{
    if (m_shutdown || ShouldClose())
    {
        return;
    }

    {
        FrameProfilerScope scope("RenderFrame");
        RenderFrame();
    }

    if (m_takePanorama)
    {
        m_takePanorama = false;
        TakePanorama();
    }

    if (m_takeScreenshot)
    {
        m_takeScreenshot = false;
        TakeScreenshot();
    }

    {
        FrameProfilerScope scope("SwapBuffers");
        RenderDevice::GetInstance()->Present(m_window.GetHandle());
    }
}

void Everstead::RequestClose()
{
    m_window.Hide();
    m_window.RequestClose();
}

bool Everstead::ShouldClose() const
{
    return m_shutdown || m_window.ShouldClose();
}

void Everstead::Shutdown()
{
    if (m_shutdown)
    {
        return;
    }

    SetMouseLock(false);
    m_window.ReleaseInputCapture();

    m_shutdown = true;
    m_discordPresence.Stop();

    if (m_loadThread.joinable())
    {
        m_loadThread.join();
    }

    if (m_saveThread.joinable())
    {
        m_saveThread.join();
    }

    SetServerAttached(false);
    if (m_serverThread.joinable())
    {
        {
            std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
            m_serverThreadRunning.store(false);
        }

        m_serverCondition.notify_one();
        m_serverThread.join();
    }

    if (m_chunkManager)
    {
        m_chunkManager->Stop();
        delete m_chunkManager;
        m_chunkManager = nullptr;
    }

    EversteadServer::DestroyInstance();

    if (m_levelRenderer)
    {
        delete m_levelRenderer;
        m_levelRenderer = nullptr;
    }

    if (m_saveService)
    {
        m_saveService->Stop();
        delete m_saveService;
        m_saveService = nullptr;
    }

    if (m_soundEngine)
    {
        m_soundEngine->Destroy();
        delete m_soundEngine;
        m_soundEngine = nullptr;
    }

    SoundRegistry::Clear();

    delete m_imguiSystem;
    m_imguiSystem = nullptr;

    delete m_uiController;
    m_uiController = nullptr;

    delete m_gui;
    m_gui = nullptr;

    delete m_inputManager;
    m_inputManager = nullptr;

    if (m_level)
    {
        delete m_level;
        m_level = nullptr;
    }

    if (m_worldLevel)
    {
        // Freeing every server chunk takes long enough to hitch the menu, so we do it on its own thread
        std::thread(
                [worldLevel = m_worldLevel]()
                {
                    delete worldLevel;
                })
                .detach();
        m_worldLevel = nullptr;
    }

    m_localPlayer = nullptr;

    if (m_camera)
    {
        delete m_camera;
        m_camera = nullptr;
    }

    delete m_defaultFont;
    m_defaultFont = nullptr;

    delete m_gameRenderer;
    m_gameRenderer = nullptr;

    delete m_frameFramebuffer;
    m_frameFramebuffer = nullptr;

    delete m_timer;
    m_timer = nullptr;

    delete m_multiplayerSession;
    m_multiplayerSession = nullptr;

    delete m_accountService;
    m_accountService = nullptr;

    TextureRepository::GetInstance()->Clear();
}

int Everstead::GetWidth() const
{
    return m_width;
}

int Everstead::GetHeight() const
{
    return m_height;
}

void Everstead::SetVSync(bool enabled)
{
    options.vsync = enabled;
    RenderDevice::GetInstance()->SetVSync(enabled);
}

void Everstead::SetWindowMode(int mode)
{
    options.windowMode = mode;
    m_window.SetMode((eWINDOW_MODE) mode);
}

void Everstead::SaveOptions()
{
    for (size_t i = 0; i < g_keyBindingCount; i++)
    {
        options.keyBindings[i] = m_inputManager->GetBinding((eKEY_BINDING) i);
    }

    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        options.controllerBindings[i] = m_inputManager->GetControllerBinding((eCONTROLLER_BINDING) i);
    }

    options.Save();
}

void Everstead::ApplyDisplayOptions()
{
    m_gameRenderer->SetGamma(options.gamma);
    RenderDevice::GetInstance()->SetVSync(options.vsync);
    m_window.SetMode((eWINDOW_MODE) options.windowMode);
    m_gameRenderer->SetFov(options.fov);
    ApplyTextureOptions();
    ApplyMeshingOptions();
    SetDiscordRichPresence(options.discordRichPresence);
}

void Everstead::SetDiscordRichPresence(bool enabled)
{
    options.discordRichPresence = enabled;
    if (enabled)
    {
        m_lastPresenceUpdateMs = 0;
        m_discordPresence.Start();
        return;
    }

    m_discordPresence.Stop();
}

eMUSIC_TYPE Everstead::GetMusicType() const
{
    if (!m_inWorld)
    {
        return eMusicType_Menu;
    }

    if (!m_level || !m_localPlayer)
    {
        return eMusicType_Game;
    }

    glm::dvec3 eyePos = m_localPlayer->GetEyePosition(1.0f);
    if (eyePos.y < 0.0)
    {
        return eMusicType_Void;
    }

    int x              = (int) floor(eyePos.x);
    int z              = (int) floor(eyePos.z);
    const Chunk *chunk = ((const Level *) m_level)->GetChunk(ChunkPos(Mth::FloorDiv(x, Chunk::s_sizeX), Mth::FloorDiv(z, Chunk::s_sizeZ)));
    if (!chunk)
    {
        return eMusicType_Game;
    }

    Biome *biome = chunk->GetBiomeAt(Mth::FloorMod(x, Chunk::s_sizeX), (int) floor(eyePos.y), Mth::FloorMod(z, Chunk::s_sizeZ));
    if (biome == Biome::ByName("ice_caves"))
    {
        return eMusicType_IceCaves;
    }
    else if (biome == Biome::ByName("desert_caves"))
    {
        return eMusicType_DesertCaves;
    }

    return biome->IsUnderground() ? eMusicType_Cave : eMusicType_Game;
}

void Everstead::UpdateDiscordPresence()
{
    uint32_t nowMs = Time::GetMilliseconds();
    if (!options.discordRichPresence || (m_lastPresenceUpdateMs != 0 && nowMs - m_lastPresenceUpdateMs < s_presenceUpdateIntervalMs))
    {
        return;
    }

    m_lastPresenceUpdateMs = nowMs;
    if (m_inWorld != m_presenceInWorld)
    {
        m_presenceInWorld      = m_inWorld;
        m_presenceStartSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    nlohmann::json activity;
    activity["assets"]["large_image"] = "logo";
    activity["assets"]["large_text"]  = "Everstead";
    activity["timestamps"]["start"]   = m_presenceStartSeconds;
    if (m_loadingPhase != eLoadingPhase_None)
    {
        activity["details"] = StringUtils::ToUtf8(GetString("discord.loading"));
    }
    else if (!m_inWorld)
    {
        activity["details"] = StringUtils::ToUtf8(GetString("discord.menu"));
    }
    else if (!m_multiplayerSession->IsOnline())
    {
        activity["details"] = StringUtils::ToUtf8(GetString("discord.singleplayer"));
        activity["state"]   = m_worldName;
    }
    else
    {
        const MultiplayerSession::SessionInfo &session = m_multiplayerSession->GetCurrentSession();
        ClientConnection *connection                   = m_localPlayer->GetClientConnection();
        int playerCount                                = connection ? (int) connection->GetPlayerList().size() : session.currentPlayers;
        activity["details"] = StringUtils::ToUtf8(GetString(m_multiplayerSession->IsHostingSession() ? "discord.hosting" : "discord.multiplayer"));
        activity["state"]   = session.name;
        activity["party"]["id"]   = std::to_string(session.sid);
        activity["party"]["size"] = {std::max(1, playerCount), std::max(1, session.maxPlayers)};
    }

    m_discordPresence.SetActivity(activity.dump());
}

int Everstead::GetChunkWorkerThreadsPerPool() const
{
    int totalThreads = options.chunkWorkerThreads > 0 ? options.chunkWorkerThreads : (int) std::thread::hardware_concurrency() - 2;
    return std::max(1, totalThreads / 2);
}

void Everstead::ApplyTextureOptions()
{
    RenderDevice *device = RenderDevice::GetInstance();
    float anisotropy     = std::min((float) (1 << options.anisotropicFiltering), device->GetMaxTextureAnisotropy());
    Texture *atlases[]   = {BlockAtlas::GetInstance()->GetBlockAtlas()->GetTexture(), BlockAtlas::GetInstance()->GetFlowAtlas()->GetTexture()};
    for (size_t i = 0; i < 2; i++)
    {
        RenderDevice::GetInstance()->SetTextureMaxLevel(atlases[i]->GetId(), std::clamp(options.mipmapLevels, 0, TextureAtlas::s_mipLevelCount - 1));
        device->SetTextureMaxAnisotropy(atlases[i]->GetId(), anisotropy);
    }
}

void Everstead::ApplyMeshingOptions()
{
    ChunkMesher::s_ambientOcclusionEnabled = options.ambientOcclusion;
    ChunkMesher::s_smartLeavesEnabled      = options.smartLeaves;
    if (m_levelRenderer)
    {
        m_levelRenderer->RemeshAll();
    }
}

void Everstead::SetRenderDistance(int distance)
{
    options.renderDistance = (float) distance;

    if (m_level)
    {
        m_level->SetRenderDistance(distance);
    }

    if (m_worldLevel)
    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        m_worldLevel->SetRenderDistance(distance);
    }
}

void Everstead::SetSimulationDistance(int distance)
{
    options.simulationDistance = distance;

    if (m_worldLevel)
    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        m_worldLevel->SetSimulationDistance(distance);
    }
}

void Everstead::CaptureWorldIcon(const std::string &worldName)
{
    int width  = m_width;
    int height = m_height;
    if (width <= 0 || height <= 0)
    {
        return;
    }

    std::vector<uint8_t> pixels((size_t) width * (size_t) height * 4);
    RenderDevice::GetInstance()->ReadPixels(0, 0, width, height, pixels.data());

    // Downsampling and the PNG write run off the render thread so saving never stalls a frame
    std::thread(
            [pixels = std::move(pixels), width, height, worldName]()
            {
                int side    = width < height ? width : height;
                int offsetX = (width - side) / 2;
                int offsetY = (height - side) / 2;

                static const int iconSize = 256;
                std::vector<uint8_t> icon((size_t) iconSize * (size_t) iconSize * 3);

                for (int y = 0; y < iconSize; y++)
                {
                    int topY0 = offsetY + (y * side) / iconSize;
                    int topY1 = offsetY + ((y + 1) * side) / iconSize;
                    if (topY1 <= topY0)
                    {
                        topY1 = topY0 + 1;
                    }

                    for (int x = 0; x < iconSize; x++)
                    {
                        int x0 = offsetX + (x * side) / iconSize;
                        int x1 = offsetX + ((x + 1) * side) / iconSize;
                        if (x1 <= x0)
                        {
                            x1 = x0 + 1;
                        }

                        uint32_t sumR    = 0;
                        uint32_t sumG    = 0;
                        uint32_t sumB    = 0;
                        uint32_t samples = 0;
                        for (int ty = topY0; ty < topY1; ty++)
                        {
                            int bottomRow = (height - 1) - ty;
                            if (bottomRow < 0 || bottomRow >= height)
                            {
                                continue;
                            }

                            for (int sx = x0; sx < x1; sx++)
                            {
                                if (sx < 0 || sx >= width)
                                {
                                    continue;
                                }

                                size_t index = ((size_t) bottomRow * (size_t) width + (size_t) sx) * 4;
                                sumR += pixels[index];
                                sumG += pixels[index + 1];
                                sumB += pixels[index + 2];
                                samples++;
                            }
                        }

                        size_t destination = ((size_t) y * (size_t) iconSize + (size_t) x) * 3;
                        if (samples > 0)
                        {
                            icon[destination]     = (uint8_t) (sumR / samples);
                            icon[destination + 1] = (uint8_t) (sumG / samples);
                            icon[destination + 2] = (uint8_t) (sumB / samples);
                        }
                    }
                }

                if (WorldSave::SaveIcon(worldName, icon, iconSize, iconSize))
                {
                    Logger::LogInfo("World icon captured for %s", worldName.c_str());
                }
            })
            .detach();
}

void Everstead::StartWorldLoad()
{
    EversteadWorldLoader::StartWorldLoad(this);
}

void Everstead::StartNewWorld(const std::string &worldName, const std::string &seed, int difficulty, int gameType)
{
    m_gui->ClearMessages();
    EversteadWorldLoader::StartNewWorld(this, worldName, seed, difficulty, gameType);
}

void Everstead::StartJoinedWorld(uint64_t sid, const std::string &worldName)
{
    m_gui->ClearMessages();
    EversteadWorldLoader::StartJoinedWorld(this, sid, worldName);
}

void Everstead::StartSavedWorld(const std::string &worldName, bool online)
{
    m_gui->ClearMessages();
    EversteadWorldLoader::StartSavedWorld(this, worldName, online);
}

void Everstead::OpenLoadWorldMenu(const std::string &worldName)
{
    WorldInfo info;
    info.name  = worldName;
    info.seed  = 0;
    info.valid = false;
    WorldSave::ReadInfo(worldName, &info);

    m_selectedWorldName = info.name;
    m_selectedWorldSeed = info.seed;
    m_uiController->NavigateToScene(eUIScene_LoadWorldMenu);
}

const std::string &Everstead::GetSelectedWorldName() const
{
    return m_selectedWorldName;
}

uint32_t Everstead::GetSelectedWorldSeed() const
{
    return m_selectedWorldSeed;
}

GameRules &Everstead::GetNewWorldGameRules()
{
    return m_newWorldGameRules;
}

WorldGenSettings &Everstead::GetNewWorldGenSettings()
{
    return m_newWorldGenSettings;
}

bool Everstead::IsWorldLoading() const
{
    return EversteadWorldLoader::IsWorldLoading(this);
}

void Everstead::OpenPauseMenu()
{
    if (!m_inWorld)
    {
        return;
    }

    if (m_gui->IsChatOpen())
    {
        m_gui->CloseChat();
    }

    m_uiController->NavigateToScene(eUIScene_PauseMenu);

    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();

    SetMouseLock(false);
}

void Everstead::OpenInventory(eUISCENE sceneId)
{
    if (!m_inWorld)
    {
        return;
    }

    m_uiController->OpenScene(sceneId);

    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();
    SetMouseLock(false);
}

void Everstead::CloseInventory()
{
    m_uiController->CloseScene();

    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();

    if (m_inWorld)
    {
        SetMouseLock(true);
    }
}

bool Everstead::IsInventoryOpen() const
{
    return m_uiController->IsSceneOpen(eUIScene_Inventory);
}

void Everstead::ClosePauseMenu()
{
    if (!m_inWorld || !IsWorldMenuOpen())
    {
        return;
    }

    UIScene *topScene = m_uiController->GetTopScene();
    while (topScene && topScene->GetId() != eUIScene_HUD && topScene->GetId() != eUIScene_DebugOverlay)
    {
        m_uiController->CloseScene();
        UIScene *nextTopScene = m_uiController->GetTopScene();
        if (nextTopScene == topScene)
        {
            break;
        }

        topScene = nextTopScene;
    }

    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();

    SetMouseLock(!IsWorldMenuOpen());
}

void Everstead::RequestLeaveWorld(bool save)
{
    if (!m_inWorld)
    {
        return;
    }

    SetMouseLock(false);
    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();
    m_leaveWorldSave = save && !kStickFight;
    if (m_leaveWorldSave && m_multiplayerSession->IsHostingSession())
    {
        m_captureWorldIcon = true;
        m_iconWorldName    = GetSaveWorldName();
    }

    m_leaveWorldRequested     = true;
    m_leaveWorldScreenVisible = false;
    OpenLeaveWorldLoadingScene();
}

void Everstead::JoinAfterLeavingWorld(uint64_t sid, const std::string &worldName)
{
    m_pendingJoinSid       = sid;
    m_pendingJoinWorldName = worldName;
    RequestLeaveWorld(true);
}

void Everstead::LeaveWorld()
{
    if (!m_inWorld)
    {
        return;
    }

    SetMouseLock(false);
    SetTickingState(20.0f, false);
    m_tickRateManager.SetFrozenTicksToRun(0);
    m_stickFightScores.fill(-1);
    m_leaveWorldRequested     = false;
    m_leaveWorldScreenVisible = false;

    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();
    m_gui->ClearMessages();
    m_soundEngine->StopWorldSounds();
    m_gameRenderer->GetDimensionTransition()->Stop();
    SetServerAttached(false);

    if (m_chunkManager)
    {
        m_chunkManager->Stop();
    }

    if (m_leaveWorldSave && m_multiplayerSession->IsHostingSession() && m_worldLevel && m_saveService)
    {
        if (!m_uiController->FindScene(eUIScene_WorldLoading))
        {
            OpenLeaveWorldLoadingScene();
        }

        std::string worldName                                     = GetSaveWorldName();
        PlayerList *playerList                                    = EversteadServer::GetInstance()->GetPlayerList();
        const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
        for (size_t i = 0; i < players.size(); i++)
        {
            players[i]->GetContainerMenu()->CloseAll();
        }

        m_saveProgress.store(0.0f);
        m_saveFinished.store(false);
        m_leavingWorld = true;
        m_saveThread   = std::thread(
                [this, worldName, playerList]()
                {
                    m_saveService->Flush();
                    WorldSave::Save(m_worldLevel, worldName, playerList, &m_saveProgress);
                    EversteadServer::GetInstance()->SaveDimensions();
                    m_saveFinished.store(true);
                });

        return;
    }

    FinishLeaveWorld();
}

void Everstead::FinishLeaveWorld()
{
    SetServerAttached(false);
    m_leavingWorld            = false;
    m_leaveWorldScreenVisible = false;

    if (m_saveThread.joinable())
    {
        m_saveThread.join();
    }

    m_saving.store(false);
    m_saveFinished.store(false);
    m_ticksSinceAutosave   = 0;
    m_backgroundSaveActive = false;
    m_saveRequested.store(false);
    m_backgroundSaver.Clear();
    m_chunkUnloader.Reset();
    m_worldLock.Release();

    // Autosaves and chunk unloads already wrote this world, so a world that was never saved by hand gets removed again
    if (!m_leaveWorldSave && !m_worldSaved && m_multiplayerSession->IsHostingSession())
    {
        m_saveService->Flush();
        WorldSave::DeleteWorld(GetSaveWorldName());
    }

    m_leaveWorldSave = true;
    m_worldSaved     = false;

    if (m_multiplayerSession->IsHostingSession())
    {
        EversteadServer::GetInstance()->GetPlayerList()->DisconnectAll(eDisconnectReason_ServerStopping);
    }

    m_inWorld = false;
    m_multiplayerSession->LeaveRoom();
    m_worldName.clear();
    m_worldLoadName.clear();
    m_selectedWorldName.clear();
    m_selectedWorldSeed    = 0;
    m_loadingExistingWorld = false;
    m_loadingOnlineWorld   = false;
    m_joiningOnlineWorld   = false;
    m_joinSessionId        = 0;

    if (m_chunkManager)
    {
        m_chunkManager->Stop();
        delete m_chunkManager;
        m_chunkManager = nullptr;
    }

    if (m_levelRenderer)
    {
        delete m_levelRenderer;
        m_levelRenderer = nullptr;
    }

    // Freeing every chunk takes long enough to hitch the menu, so both levels are destroyed on their own thread
    std::thread(
            [level = m_level, worldLevel = m_worldLevel]()
            {
                delete level;
                delete worldLevel;
            })
            .detach();
    m_level      = nullptr;
    m_worldLevel = nullptr;

    m_localPlayer = nullptr;

    if (m_camera)
    {
        delete m_camera;
        m_camera = nullptr;
    }

    m_uiController->ClearScenes();
    m_uiController->OpenScene(eUIScene_MainMenu);

    if (m_pendingJoinSid != 0)
    {
        StartJoinedWorld(m_pendingJoinSid, m_pendingJoinWorldName);
        m_pendingJoinSid = 0;
    }
}

void Everstead::ShowDisconnectReason(eDISCONNECT_REASON reason, const std::wstring &text)
{
    UIScene_ModalDialog::Dialog dialog;
    dialog.title   = GetString("dialog.disconnected.title");
    dialog.message = GetString(DisconnectS2CPacket::GetReasonKey(reason));
    if (reason == eDisconnectReason_OlderVersion || reason == eDisconnectReason_NewerVersion)
    {
        wchar_t message[256];
        swprintf(message, 256, dialog.message.c_str(), g_gameVersionMajor, g_gameVersionMinor, g_gameVersionPatch, text.c_str());
        dialog.message = message;
    }
    else if (reason == eDisconnectReason_Kicked && !text.empty())
    {
        dialog.message = text;
    }

    UIScene_ModalDialog::Button button;
    button.label = GetString("button.ok");
    dialog.buttons.push_back(button);

    m_uiController->ShowModal(dialog);
}

void Everstead::AddBannedSession(uint64_t sid)
{
    if (sid == 0 || IsSessionBanned(sid))
    {
        return;
    }

    m_bannedSessions.push_back(sid);
}

bool Everstead::IsSessionBanned(uint64_t sid) const
{
    for (size_t i = 0; i < m_bannedSessions.size(); i++)
    {
        if (m_bannedSessions[i] == sid)
        {
            return true;
        }
    }

    return false;
}

void Everstead::SaveWorld()
{
    if (!CanSaveWorld())
    {
        return;
    }

    std::string worldName = GetSaveWorldName();
    m_worldSaved          = true;
    m_captureWorldIcon    = true;
    m_iconWorldName       = worldName;
    m_saveRequested.store(true);
}

bool Everstead::IsSaving() const
{
    return m_saveService && (m_saveService->IsBusy() || m_backgroundSaver.HasPendingChunks());
}

int Everstead::GetTicksUntilAutosave() const
{
    if (!m_multiplayerSession->IsHostingSession() || options.autosaveIntervalTicks <= 0)
    {
        return -1;
    }

    int remaining = options.autosaveIntervalTicks - m_ticksSinceAutosave;
    return remaining < 0 ? 0 : remaining;
}

void Everstead::ResetAutosaveTimer()
{
    m_ticksSinceAutosave = 0;
}

void Everstead::BackgroundSave()
{
    if (!m_inWorld || !m_worldLevel || !m_multiplayerSession->IsHostingSession() || !m_saveService || kStickFight)
    {
        return;
    }

    if (!m_backgroundSaveActive)
    {
        m_backgroundSaveStartMs = Time::GetMilliseconds();
    }

    m_backgroundSaveActive = true;
    m_backgroundSaver.Begin(EversteadServer::GetInstance(), m_saveService, GetSaveWorldName());
}

void Everstead::SetServerAttached(bool attached)
{
    if (m_serverAttached.load() == attached)
    {
        return;
    }

    if (attached && !m_serverThread.joinable())
    {
        m_serverThreadRunning.store(true);
        m_serverThread = std::thread(&Everstead::RunServerThread, this);
    }

    {
        std::lock_guard<std::mutex> lock(EversteadServer::GetInstance()->GetMutex());
        m_serverAttached.store(attached);
    }

    m_serverCondition.notify_one();
}

void Everstead::RunServerThread()
{
    std::mutex &mutex                              = EversteadServer::GetInstance()->GetMutex();
    std::chrono::steady_clock::time_point nextTick = std::chrono::steady_clock::now();
    while (m_serverThreadRunning.load())
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!m_serverAttached.load())
            {
                m_serverCondition.wait(lock,
                                       [this]()
                                       {
                                           return m_serverAttached.load() || !m_serverThreadRunning.load();
                                       });
                nextTick = std::chrono::steady_clock::now();
                continue;
            }

            TickServerThread();
            nextTick += EversteadServer::GetInstance()->GetTickRateManager()->GetTickInterval();
            if (EversteadServer::GetInstance()->IsSprinting())
            {
                nextTick = std::chrono::steady_clock::now();
            }
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        // We skip the missed ticks after a long stall instead of running them back to back
        if (now - nextTick > std::chrono::seconds(2))
        {
            nextTick = now;
        }

        std::this_thread::sleep_until(nextTick);
    }
}

void Everstead::ApplyServerSettings()
{
    ServerSettings &settings     = EversteadServer::GetInstance()->settings;
    settings.chunkWorkerThreads  = options.chunkWorkerThreads;
    settings.chunkIntakePerTick  = options.chunkIntakePerTick;
    settings.chunkSendsPerTick   = options.chunkSendsPerTick;
    settings.chunkSendsInFlight  = options.chunkSendsInFlight;
    settings.chunkQueuePerThread = options.chunkQueuePerThread;
    settings.lodEnabled          = options.lodEnabled;
    settings.lodCpuPreset        = options.lodCpuPreset;
    settings.hostPid             = m_multiplayerSession->GetClientPid();
}

void Everstead::TickServerThread()
{
    ApplyServerSettings();
    EversteadServer *server = EversteadServer::GetInstance();
    if (m_serverSimulating.load())
    {
        server->Tick();

        std::vector<glm::dvec3> positions;
        server->GetPlayerList()->GetPlayerPositions(m_worldLevel, &positions);
        if (positions.empty())
        {
            positions.push_back(m_playerSpawnPosition);
        }

        m_chunkManager->Update(positions);
        LodGenerator::GetInstance()->SetChunkBacklog((int) (m_chunkManager->GetPendingCount() + m_chunkManager->GetActiveCount()));
        m_chunkUnloader.UnloadServerChunks(m_worldLevel, positions, m_worldLevel->GetRenderDistance() + options.chunkUnloadMargin, m_saveService,
                                           GetSaveWorldName());
        server->UpdateDimensions(m_saveService, options.chunkUnloadMargin);
        server->UpdateLighting();

        if (options.autosaveIntervalTicks > 0 && ++m_ticksSinceAutosave >= options.autosaveIntervalTicks)
        {
            m_ticksSinceAutosave = 0;
            BackgroundSave();
        }
    }

    if (m_saveRequested.exchange(false))
    {
        BackgroundSave();
    }

    m_backgroundSaver.Drain(m_saveService, GetSaveWorldName());
    if (m_backgroundSaveActive && !IsSaving())
    {
        uint32_t elapsedMs = Time::GetMilliseconds() - m_backgroundSaveStartMs;
        if (options.announceWorldSaves)
        {
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
            server->GetPlayerList()->BroadcastChat(component);
        }

        m_backgroundSaveActive = false;
    }
}

std::string Everstead::GetSaveWorldName() const
{
    return m_worldName.empty() ? "New World" : m_worldName;
}

bool Everstead::CanSaveWorld() const
{
    return m_inWorld && !m_saving.load() && m_multiplayerSession->IsHostingSession() && !kStickFight;
}

void Everstead::UpdateSaving()
{
    if (!m_saving.load())
    {
        return;
    }

    UIScene *savingScene = m_uiController->FindScene(eUIScene_WorldSaving);
    if (savingScene)
    {
        ((UIScene_WorldSaving *) savingScene)->SetStatus(GetString("saving.writing_regions"));
        ((UIScene_WorldSaving *) savingScene)->SetProgress(m_saveProgress.load());
    }

    if (!m_saveFinished.load())
    {
        return;
    }

    if (m_saveThread.joinable())
    {
        m_saveThread.join();
    }

    EversteadServer::GetInstance()->SetSaving(false);
    m_saving.store(false);
    CloseSavingScene();

    if (m_localPlayer)
    {
        m_localPlayer->ClearInputs();
    }

    m_inputManager->Clear();
}

void Everstead::OpenSavingScene(const std::wstring &status, float progress)
{
    UIScene *topScene = m_uiController->GetTopScene();
    while (topScene && topScene->GetId() != eUIScene_HUD && topScene->GetId() != eUIScene_DebugOverlay && topScene->GetId() != eUIScene_WorldSaving)
    {
        m_uiController->CloseScene();
        UIScene *nextTopScene = m_uiController->GetTopScene();
        if (nextTopScene == topScene)
        {
            break;
        }

        topScene = nextTopScene;
    }

    UIScene *savingScene = m_uiController->FindScene(eUIScene_WorldSaving);
    if (!savingScene)
    {
        savingScene = m_uiController->OpenScene(eUIScene_WorldSaving);
    }

    if (savingScene)
    {
        ((UIScene_WorldSaving *) savingScene)->SetStatus(status);
        ((UIScene_WorldSaving *) savingScene)->SetProgress(progress);
    }
}

void Everstead::CloseSavingScene()
{
    UIScene *topScene = m_uiController->GetTopScene();
    if (topScene && topScene->GetId() == eUIScene_WorldSaving)
    {
        m_uiController->CloseScene();
    }

    SetMouseLock(true);
}

void Everstead::StartDimensionTransition()
{
    UIScene *topScene = m_uiController->GetTopScene();
    while (topScene && topScene->GetId() != eUIScene_HUD && topScene->GetId() != eUIScene_DebugOverlay)
    {
        m_uiController->CloseScene();
        UIScene *nextTopScene = m_uiController->GetTopScene();
        if (nextTopScene == topScene)
        {
            break;
        }

        topScene = nextTopScene;
    }

    SetMouseLock(true);
    m_gameRenderer->GetDimensionTransition()->Start();
}

void Everstead::FinishDimensionTransition()
{
    m_gameRenderer->GetDimensionTransition()->Finish();
}

void Everstead::OpenLeaveWorldLoadingScene()
{
    m_uiController->ClearScenes();
    UIScene *loadingScene = m_uiController->FindScene(eUIScene_WorldLoading);
    if (!loadingScene)
    {
        loadingScene = m_uiController->OpenScene(eUIScene_WorldLoading);
    }

    if (loadingScene)
    {
        ((UIScene_WorldLoading *) loadingScene)->SetTitle(GetString("loading.leaving_world"));
        ((UIScene_WorldLoading *) loadingScene)->SetStatus(GetString("loading.saving_and_closing"));
        ((UIScene_WorldLoading *) loadingScene)->SetProgress(1.0f);
    }
}

void Everstead::BeginWorldInit()
{
    EversteadWorldLoader::BeginWorldInit(this);
}

void Everstead::UpdateWorldLoading()
{
    EversteadWorldLoader::UpdateWorldLoading(this);
}

void Everstead::FinalizeWorldLoad()
{
    EversteadWorldLoader::FinalizeWorldLoad(this);
}

bool Everstead::IsWorldMenuOpen() const
{
    if (!m_inWorld)
    {
        return false;
    }

    UIScene *topScene = m_uiController->GetTopScene();
    if (!topScene || !topScene->IsVisible())
    {
        return false;
    }

    return topScene->GetId() != eUIScene_HUD && topScene->GetId() != eUIScene_DebugOverlay;
}

Timer *Everstead::GetTimer() const
{
    return m_timer;
}

TickRateManager *Everstead::GetTickRateManager()
{
    return &m_tickRateManager;
}

void Everstead::SetTickingState(float tickRate, bool frozen)
{
    m_tickRateManager.SetTickRate(tickRate);
    m_tickRateManager.SetFrozen(frozen);
    m_timer->SetTicksPerSecond(tickRate);
}

const std::array<int, 4> &Everstead::GetStickFightScores() const
{
    return m_stickFightScores;
}

void Everstead::SetStickFightScores(const std::array<int, 4> &scores)
{
    m_stickFightScores = scores;
}

float Everstead::GetRenderPartialTicks() const
{
    return m_renderPartialTicks;
}

const Camera *Everstead::GetCamera() const
{
    return m_camera;
}

MultiPlayerLocalPlayer *Everstead::GetLocalPlayer() const
{
    return m_localPlayer;
}

MultiPlayerLevel *Everstead::GetLevel() const
{
    return m_level;
}

Level *Everstead::GetWorldLevel() const
{
    return m_worldLevel;
}

LevelRenderer *Everstead::GetLevelRenderer() const
{
    return m_levelRenderer;
}

ChunkManager *Everstead::GetChunkManager() const
{
    return m_chunkManager;
}

Font *Everstead::GetDefaultFont() const
{
    return m_defaultFont;
}

InputManager *Everstead::GetInputManager() const
{
    return m_inputManager;
}

AccountService *Everstead::GetAccountService() const
{
    return m_accountService;
}

MultiplayerSession *Everstead::GetMultiplayerSession() const
{
    return m_multiplayerSession;
}

UIController *Everstead::GetUiController() const
{
    return m_uiController;
}

Gui *Everstead::GetGui() const
{
    return m_gui;
}

GameRenderer *Everstead::GetGameRenderer() const
{
    return m_gameRenderer;
}

Framebuffer *Everstead::GetFrameFramebuffer() const
{
    return m_frameFramebuffer;
}

SoundEngine *Everstead::GetSoundEngine() const
{
    return m_soundEngine;
}

MusicManager *Everstead::GetMusicManager()
{
    return &m_musicManager;
}

std::wstring Everstead::GetString(const std::string &key) const
{
    return Localization::GetInstance()->Get(key);
}

void Everstead::RenderFrame()
{
    SyncRenderSizeFromWindow();

    bool imguiVisible = m_imguiSystem->IsVisible();
    if (imguiVisible)
    {
        FrameProfilerScope scope("ImGui::BeginFrame");
        m_imguiSystem->BeginFrame();
    }

    m_frameFramebuffer->Bind();
    RenderDevice::GetInstance()->ClearAll();

    if (m_inWorld && m_levelRenderer)
    {
        FrameProfilerScope scope("LevelRenderer::Render");
        m_levelRenderer->Render(m_renderPartialTicks, m_frameFramebuffer);
    }

    if (m_captureWorldIcon)
    {
        m_captureWorldIcon = false;
        if (m_inWorld)
        {
            CaptureWorldIcon(m_iconWorldName);
        }
    }

    GlStateManager::DisableFog();

    GlStateManager::MatrixMode(1);
    GlStateManager::LoadIdentity();
    GlStateManager::Ortho(0.0f, (float) m_width, (float) m_height, 0.0f, -1.0f, 1.0f);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();

    {
        FrameProfilerScope scope("UIController::Render");
        m_uiController->Render();
    }

    if (options.animatedTextures)
    {
        ItemAtlas::GetInstance()->GetAtlas()->UpdateAnimations(Time::GetDelta(), {});
    }

    m_frameFramebuffer->Unbind();

    {
        FrameProfilerScope scope("GammaPass");
        m_gameRenderer->RenderGammaPass(m_frameFramebuffer);
    }

    if (imguiVisible)
    {
        FrameProfilerScope scope("ImGui::Render");
        if (m_inWorld && m_levelRenderer)
        {
            m_levelRenderer->RenderNimbusImGui();
        }

        m_imguiSystem->Render();
    }
}

void Everstead::SetMouseLock(bool locked)
{
    m_mouseLocked = m_inWorld && locked;

    int centerX = m_width / 2;
    int centerY = m_height / 2;

    if (m_mouseLocked)
    {
        SDL_WarpMouseInWindow(m_window.GetHandle(), centerX, centerY);
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_SetWindowGrab(m_window.GetHandle(), SDL_TRUE);
        ResetSdlMouseState();
    }
    else
    {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_SetWindowGrab(m_window.GetHandle(), SDL_FALSE);
        SDL_WarpMouseInWindow(m_window.GetHandle(), centerX, centerY);
        ResetSdlMouseState();
    }
}

void Everstead::ReportChunkGenerationFailure(const ChunkPos &pos)
{
    if (!MemoryMonitor::GetInstance()->ConsumeNotification())
    {
        return;
    }

    Logger::LogError("Could not generate the chunk at %d, %d", pos.x, pos.z);

    if (!m_uiController)
    {
        return;
    }

    wchar_t message[0xFF];
    swprintf(message, 0xFF, GetString("chunk.error.generation_failed").c_str(), pos.x, pos.z);
    m_uiController->ShowNotification(GetString("chunk.error.title"), message, 6.0f);
}

void Everstead::ToggleDebugOverlay()
{
    ToggleDebugOverlay(false);
}

void Everstead::ToggleDebugOverlay(bool withProfiler)
{
    UIScene *scene = m_uiController->FindScene(eUIScene_DebugOverlay);
    if (!m_inWorld || !scene)
    {
        return;
    }

    UIScene_DebugOverlay *debugOverlay = (UIScene_DebugOverlay *) scene;
    if (scene->IsVisible())
    {
        debugOverlay->SetProfilerVisible(false);
        scene->SetVisible(false);
        return;
    }

    debugOverlay->SetProfilerVisible(withProfiler);
    scene->SetVisible(true);
}

bool Everstead::IsHudVisible() const
{
    return !m_hudHidden || m_gui->IsChatOpen();
}

void Everstead::TakeScreenshot()
{
    RenderDevice::GetInstance()->BindFramebuffer({});
    std::vector<uint8_t> rgba = Screenshot::ReadPixels(m_width, m_height);
    File target               = Screenshot::CreateTarget(".png");
    bool saved                = target.WriteAllBytes(PngEncoder::Encode(Screenshot::ToTopDownRgb(rgba, m_width, m_height), m_width, m_height));
    Logger::LogInfo(saved ? "Saved screenshot %s" : "Failed to save screenshot %s", target.GetPath().c_str());

    nlohmann::json component;
    if (saved)
    {
        nlohmann::json nameComponent;
        nameComponent["text"]       = target.GetName();
        nameComponent["underlined"] = true;
        nameComponent["clickEvent"] = {{"action", "open_file"}, {"value", target.GetName()}};
        component["translate"]      = "screenshot.success";
        component["with"]           = nlohmann::json::array();
        component["with"].push_back(nameComponent);
    }
    else
    {
        component["translate"] = "screenshot.failure";
        component["color"]     = "red";
    }

    m_gui->AddMessage(StringUtils::ToUtf16(component.dump()), true);
}

void Everstead::TakePanorama()
{
    static const glm::dvec3 faceFronts[6] = {glm::dvec3(0.0, 0.0, 1.0),  glm::dvec3(1.0, 0.0, 0.0), glm::dvec3(0.0, 0.0, -1.0),
                                             glm::dvec3(-1.0, 0.0, 0.0), glm::dvec3(0.0, 1.0, 0.0), glm::dvec3(0.0, -1.0, 0.0)};
    static const glm::dvec3 faceUps[6]    = {glm::dvec3(0.0, 1.0, 0.0), glm::dvec3(0.0, 1.0, 0.0),  glm::dvec3(0.0, 1.0, 0.0),
                                             glm::dvec3(0.0, 1.0, 0.0), glm::dvec3(0.0, 0.0, -1.0), glm::dvec3(0.0, 0.0, 1.0)};

    glm::dvec3 front = m_camera->GetFront();
    glm::dvec3 forward(front.x, 0.0, front.z);
    forward = glm::length2(forward) > 0.000001 ? glm::normalize(forward) : glm::dvec3(0.0, 0.0, -1.0);
    glm::dvec3 right(forward.z, 0.0, -forward.x);
    glm::dvec3 up(0.0, 1.0, 0.0);

    File directory = Screenshot::CreateTarget("");
    directory.CreateDirectories();

    int size = std::min(m_width, m_height);
    Framebuffer sceneTarget(m_width, m_height);
    Framebuffer gammaTarget(m_width, m_height);
    float gamma = m_gameRenderer->GetGamma();
    m_gameRenderer->SetGamma(kPanoramaGamma);
    m_gameRenderer->SetPanoramaCapture(true);
    for (int face = 0; face < 6; face++)
    {
        glm::dvec3 faceFront = right * faceFronts[face].x + up * faceFronts[face].y + forward * faceFronts[face].z;
        glm::dvec3 faceUp    = right * faceUps[face].x + up * faceUps[face].y + forward * faceUps[face].z;
        m_camera->SetDirection(faceFront, faceUp);

        sceneTarget.Bind();
        RenderDevice::GetInstance()->ClearAll();
        m_levelRenderer->Render(m_renderPartialTicks, &sceneTarget);
        gammaTarget.Bind();
        m_gameRenderer->RenderGammaPass(&sceneTarget);

        std::vector<uint8_t> rgba = Screenshot::ReadPixels(m_width, m_height);
        File target               = directory.GetChild("panorama_" + std::to_string(face) + ".png");
        target.WriteAllBytes(PngEncoder::Encode(Screenshot::ToPanoramaFace(rgba, m_width, m_height, size), size, size));
    }

    gammaTarget.Unbind();
    m_gameRenderer->SetPanoramaCapture(false);
    m_gameRenderer->SetGamma(gamma);
    m_gameRenderer->UpdateCamera(m_camera, m_localPlayer, m_renderPartialTicks);
    Logger::LogInfo("Saved panorama to %s", directory.GetPath().c_str());

    nlohmann::json nameComponent;
    nameComponent["text"]       = directory.GetName();
    nameComponent["underlined"] = true;

    nlohmann::json component;
    component["translate"] = "screenshot.panorama";
    component["with"]      = nlohmann::json::array();
    component["with"].push_back(nameComponent);
    m_gui->AddMessage(StringUtils::ToUtf16(component.dump()), true);
}

void Everstead::ToggleHud()
{
    if (m_inWorld)
    {
        m_hudHidden = !m_hudHidden;
    }
}

void Everstead::ToggleNimbusMenu()
{
    if (!m_inWorld || !kNimbusEnabled || RenderDevice::GetInstance()->GetBackend() == eRenderBackend_OpenGL)
    {
        return;
    }

    m_imguiSystem->ToggleVisible();
    if (m_imguiSystem->IsVisible())
    {
        // The command usually lands while chat is still closing, so we drop the pending relock too
        m_restoreMouseLockAfterBlockingUi = false;
        SetMouseLock(false);
    }
}

void Everstead::AddDebugMessage(const std::string &translationKey)
{
    nlohmann::json component;
    component["text"]  = "";
    component["extra"] = {{{"translate", "debug.prefix"}, {"color", "yellow"}, {"bold", true}}, {{"text", " "}}, {{"translate", translationKey}}};
    m_gui->AddMessage(StringUtils::ToUtf16(component.dump()), true);
}

void Everstead::UpdateResourceLoading()
{
    bool done                   = m_resourceLoader->Update();
    UIScene_WorldLoading *scene = (UIScene_WorldLoading *) m_uiController->FindScene(eUIScene_WorldLoading);
    scene->SetStatus(m_resourceLoader->GetStatus());
    scene->SetProgress(m_resourceLoader->GetProgress());
    if (!done)
    {
        return;
    }

    delete m_resourceLoader;
    m_resourceLoader = nullptr;
    ApplyTextureOptions();
    Logger::LogInfo("Finished loading game resources");

    m_uiController->ClearScenes();
    m_uiController->OpenScene(eUIScene_MainMenu);
}

void Everstead::SyncRenderSizeFromWindow()
{
    int drawableWidth  = 0;
    int drawableHeight = 0;
    RenderDevice::GetInstance()->GetDrawableSize(m_window.GetHandle(), &drawableWidth, &drawableHeight);
    if (drawableWidth <= 0 || drawableHeight <= 0)
    {
        return;
    }

    ApplyRenderSize(drawableWidth, drawableHeight);
}

void Everstead::ApplyRenderSize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (m_width == width && m_height == height)
    {
        RenderDevice::GetInstance()->SetViewport(0, 0, width, height);
        return;
    }

    m_width  = width;
    m_height = height;

    RenderDevice::GetInstance()->SetViewport(0, 0, width, height);
    m_gameRenderer->UpdateProjection(width, height);

    if (m_levelRenderer)
    {
        m_levelRenderer->OnResize(width, height);
    }

    m_frameFramebuffer->Resize(width, height);
}

#endif
