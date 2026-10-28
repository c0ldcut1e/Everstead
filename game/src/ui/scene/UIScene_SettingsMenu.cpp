#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIScene_SettingsMenu.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <thread>

#include "../../core/Everstead.h"
#include "../../rendering/GameRenderer.h"
#include "../../rendering/RenderBackend.h"
#include "../../rendering/RenderDevice.h"
#include "../../rendering/TextureAtlas.h"
#include "../../world/lod/LodRenderer.h"
#include "../Screen.h"

static const char *const s_windowModeKeys[]    = {"options.graphics.window_mode.windowed", "options.graphics.window_mode.borderless",
                                                  "options.graphics.window_mode.fullscreen"};
static const char *const s_particleKeys[]      = {"options.graphics.particles.all", "options.graphics.particles.decreased",
                                                  "options.graphics.particles.minimal"};
static const char *const s_dynamicLightKeys[]  = {"options.graphics.dynamic_lights.off", "options.graphics.dynamic_lights.fast",
                                                  "options.graphics.dynamic_lights.fancy"};
static const char *const s_lodQualityKeys[]    = {"options.graphics.lod_quality.low", "options.graphics.lod_quality.medium",
                                                  "options.graphics.lod_quality.high", "options.graphics.lod_quality.ultra"};
static const char *const s_lodCpuPresetKeys[]  = {"options.graphics.lod_cpu_preset.low", "options.graphics.lod_cpu_preset.medium",
                                                  "options.graphics.lod_cpu_preset.high", "options.graphics.lod_cpu_preset.extreme"};
static const char *const s_nimbusQualityKeys[] = {"options.graphics.nimbus_quality.low", "options.graphics.nimbus_quality.medium",
                                                  "options.graphics.nimbus_quality.high"};
static const char *const s_fpsDisplayKeys[]    = {"options.off", "options.graphics.fps_display.basic", "options.graphics.fps_display.extended"};

std::wstring UIScene_SettingsMenu::FormatSliderText(const std::wstring &name, float value)
{
    return name + L": " + std::to_wstring((int) round(value));
}

std::wstring UIScene_SettingsMenu::FormatSliderTextPercentage(const std::wstring &name, float value)
{
    return name + L": " + std::to_wstring((int) round(value)) + L"%";
}

int UIScene_SettingsMenu::AutosaveTicksForStep(int step)
{
    static const int ticks[] = {0, 900, 1200, 6000, 12000};
    if (step < 0)
    {
        step = 0;
    }

    if (step > 4)
    {
        step = 4;
    }

    return ticks[step];
}

int UIScene_SettingsMenu::AutosaveStepForTicks(int ticks)
{
    for (int step = 0; step <= 4; step++)
    {
        if (AutosaveTicksForStep(step) == ticks)
        {
            return step;
        }
    }

    return 0;
}

std::wstring UIScene_SettingsMenu::FormatAutosaveText(int step)
{
    Everstead *everstead = Everstead::GetInstance();
    std::wstring label   = everstead->GetString("options.game.autosave_interval") + L": ";

    static const char *keys[] = {"options.off", "options.autosave.45s", "options.autosave.1m", "options.autosave.5m", "options.autosave.10m"};
    if (step < 0)
    {
        step = 0;
    }

    if (step > 4)
    {
        step = 4;
    }

    return label + everstead->GetString(keys[step]);
}

std::wstring UIScene_SettingsMenu::FormatChoiceText(const char *nameKey, const char *const *choiceKeys, int count, int index)
{
    Everstead *everstead = Everstead::GetInstance();
    return everstead->GetString(nameKey) + L": " + everstead->GetString(choiceKeys[std::clamp(index, 0, count - 1)]);
}

std::wstring UIScene_SettingsMenu::FormatZeroOrValueText(const char *nameKey, const char *zeroKey, int value, const std::wstring &suffix)
{
    Everstead *everstead = Everstead::GetInstance();
    std::wstring label   = everstead->GetString(nameKey) + L": ";
    if (value <= 0)
    {
        return label + everstead->GetString(zeroKey);
    }

    return label + std::to_wstring(value) + suffix;
}

std::wstring UIScene_SettingsMenu::FormatFpsLimitText(int step)
{
    Everstead *everstead = Everstead::GetInstance();
    std::wstring label   = everstead->GetString("options.graphics.fps_limit") + L": ";
    if (step > s_fpsLimitSteps)
    {
        return label + everstead->GetString("options.graphics.fps_limit.unlimited");
    }

    return label + std::to_wstring(step * s_fpsLimitStep);
}

UIScene_SettingsMenu::UIScene_SettingsMenu() : UIScene(eUIScene_SettingsMenu)
{
    m_groupRoot          = new UIGroup("Group_SettingsMenu");
    m_backgroundFill     = new UIComponent_Fill(0x70000000);
    m_multiList          = {new UIControl_MultiList(0.0f, 0.0f, 0.0f, 0.0f)};
    m_currentCategory    = eCategory_None;
    m_rootSelectionIndex = -1;
    m_subSelectionIndex  = -1;
    m_multiList->SetupControl(this);
    m_multiList->SetButtonPressedCallback(HandleButtonPressed, this);
    m_multiList->SetCheckboxUpdatedCallback(HandleCheckboxUpdated, this);
    m_multiList->SetSliderUpdatedCallback(HandleSliderUpdated, this);
    m_multiList->SetSelectionUpdatedCallback(UIScene::PlayFocusSound, this);

    m_backgroundFill->SetVisible(false);

    m_groupRoot->AddComponent(m_backgroundFill);
    m_groupRoot->AddComponent(m_multiList);
    AddGroup(m_groupRoot);

    ShowRootCategories();
}

UIScene_SettingsMenu::~UIScene_SettingsMenu()
{}

void UIScene_SettingsMenu::OnEnter()
{
    m_backgroundFill->SetVisible(Everstead::GetInstance()->GetLevel() != nullptr);
    ShowRootCategories();
    m_multiList->ResetPointerInputState();
}

void UIScene_SettingsMenu::OnExit()
{
    Everstead::GetInstance()->SaveOptions();
}

void UIScene_SettingsMenu::OnGameplayAction(eGAMEPLAY_ACTION action)
{
    if (action != eGameplayAction_Back)
    {
        return;
    }

    if ((m_currentCategory >= eCategory_GraphicsDisplay && m_currentCategory <= eCategory_GraphicsPerformance) ||
        m_currentCategory == eCategory_GraphicsQuality || m_currentCategory == eCategory_GraphicsLod || m_currentCategory == eCategory_GraphicsNimbus)
    {
        ShowGraphics();
        return;
    }

    if (m_currentCategory == eCategory_UserInterfaceHud || m_currentCategory == eCategory_UserInterfaceChat)
    {
        ShowUserInterface();
        return;
    }

    if (m_currentCategory != eCategory_None)
    {
        ShowRootCategories();
        return;
    }

    UIController *uiController = Everstead::GetInstance()->GetUiController();
    uiController->CloseScene();
}

void UIScene_SettingsMenu::HandleButtonPressed(void *context, uint32_t id)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    UIScene_SettingsMenu *_this = (UIScene_SettingsMenu *) context;
    if (_this->m_currentCategory == eCategory_None)
    {
        _this->m_rootSelectionIndex = _this->m_multiList->GetCurrentSelection();

        switch (id)
        {
            case s_settingsMenuGameOptionsId: {
                _this->ShowGameOptions();
                break;
            }

            case s_settingsMenuAudioId: {
                _this->ShowAudio();
                break;
            }

            case s_settingsMenuGraphicsId: {
                _this->m_subSelectionIndex = -1;
                _this->ShowGraphics();
                break;
            }

            case s_settingsMenuUserInterfaceId: {
                _this->m_subSelectionIndex = -1;
                _this->ShowUserInterface();
                break;
            }

            case s_settingsMenuResetToDefaultsId: {
                _this->ShowResetToDefaults();
                break;
            }

            default: {
                break;
            }
        }

        return;
    }

    if (_this->m_currentCategory == eCategory_Graphics || _this->m_currentCategory == eCategory_UserInterface)
    {
        _this->m_subSelectionIndex = _this->m_multiList->GetCurrentSelection();
    }

    switch (id)
    {
        case s_settingsMenuDisplayId: {
            _this->ShowGraphicsDisplay();
            break;
        }

        case s_settingsMenuRenderingId: {
            _this->ShowGraphicsRendering();
            break;
        }

        case s_settingsMenuPerformanceId: {
            _this->ShowGraphicsPerformance();
            break;
        }

        case s_settingsMenuQualityId: {
            _this->ShowGraphicsQuality();
            break;
        }

        case s_settingsMenuLodId: {
            _this->ShowGraphicsLod();
            break;
        }

        case s_settingsMenuNimbusId: {
            _this->ShowGraphicsNimbus();
            break;
        }

        case s_settingsMenuWindowModeId: {
            Everstead *everstead = Everstead::GetInstance();
            int windowMode       = (everstead->options.windowMode + 1) % 3;
            everstead->SetWindowMode(windowMode);
            _this->m_multiList->SetButtonText(id, FormatChoiceText("options.graphics.window_mode", s_windowModeKeys, 3, windowMode));
            break;
        }

        case s_settingsMenuHudId: {
            _this->ShowUserInterfaceHud();
            break;
        }

        case s_settingsMenuChatId: {
            _this->ShowUserInterfaceChat();
            break;
        }

        case s_settingsMenuLanguagesId: {
            Everstead::GetInstance()->GetUiController()->NavigateToScene(eUIScene_LanguagesMenu);
            break;
        }

        case s_settingsMenuResetCancelId: {
            _this->ShowRootCategories();
            break;
        }

        case s_settingsMenuResetOkId: {
            Everstead::GetInstance()->options.Reset();
            Everstead::GetInstance()->ApplyDisplayOptions();
            _this->ShowRootCategories();
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_SettingsMenu::HandleCheckboxUpdated(void *context, uint32_t id, bool checked)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.press");
    (void) context;

    Options *options = &Everstead::GetInstance()->options;

    switch (id)
    {
        case s_settingsMenuViewBobbingId: {
            options->viewBobbing = checked;
            break;
        }

        case s_settingsMenuAnnounceSavesId: {
            options->announceWorldSaves = checked;
            break;
        }

        case s_settingsMenuDiscordPresenceId: {
            Everstead::GetInstance()->SetDiscordRichPresence(checked);
            break;
        }

        case s_settingsMenuSoundRaytracingId: {
            options->soundRaytracing = checked;
            break;
        }

        case s_settingsMenuVsyncId: {
            Everstead::GetInstance()->SetVSync(checked);
            break;
        }

        case s_settingsMenuFrustumCullingId: {
            options->frustumCulling = checked;
            break;
        }

        case s_settingsMenuGpuChunkCullingId: {
            options->gpuChunkCulling = checked;
            break;
        }

        case s_settingsMenuSmoothLightingId: {
            options->smoothLighting = checked;
            Everstead::GetInstance()->ApplyMeshingOptions();
            break;
        }

        case s_settingsMenuNimbusEnabledId: {
            options->nimbusEnabled = checked;
            break;
        }

        case s_settingsMenuNimbusLightingId: {
            options->nimbusLighting = checked;
            break;
        }

        case s_settingsMenuNimbusCloudsId: {
            options->nimbusClouds = checked;
            break;
        }

        case s_settingsMenuNimbusFogId: {
            options->nimbusFog = checked;
            break;
        }

        case s_settingsMenuAmbientOcclusionId: {
            options->ambientOcclusion = checked;
            Everstead::GetInstance()->ApplyMeshingOptions();
            break;
        }

        case s_settingsMenuAnimatedTexturesId: {
            options->animatedTextures = checked;
            break;
        }

        case s_settingsMenuInstantBlockUpdatesId: {
            options->instantBlockUpdates = checked;
            break;
        }

        case s_settingsMenuSyncBlockUpdatesId: {
            options->synchronizedBlockUpdates = checked;
            break;
        }

        case s_settingsMenuRenderCloudsId: {
            options->renderClouds = checked;
            break;
        }

        case s_settingsMenuEntityShadowsId: {
            options->entityShadows = checked;
            break;
        }

        case s_settingsMenuSmartLeavesId: {
            options->smartLeaves = checked;
            Everstead::GetInstance()->ApplyMeshingOptions();
            break;
        }

        case s_settingsMenuFlatItemsId: {
            options->flatItems = checked;
            break;
        }

        case s_settingsMenuLodEnabledId: {
            options->lodEnabled = checked;
            break;
        }

        case s_settingsMenuLodTexturesId: {
            options->lodTextures = checked;
            break;
        }

        case s_settingsMenuLodGenerateTerrainId: {
            options->lodGenerateTerrain = checked;
            break;
        }

        case s_settingsMenuLodDebugId: {
            options->lodDebug = checked;
            break;
        }

        case s_settingsMenuOcclusionCullingId: {
            options->occlusionCulling = checked;
            break;
        }

        case s_settingsMenuFogCullingId: {
            options->fogCulling = checked;
            break;
        }

        case s_settingsMenuDisplayHudId: {
            options->displayHud = checked;
            break;
        }

        case s_settingsMenuJavaTooltipsId: {
            options->javaTooltips = checked;
            break;
        }

        case s_settingsMenuDisplayHandId: {
            options->displayHand = checked;
            break;
        }

        case s_settingsMenuChatShownId: {
            options->chatShown = checked;
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_SettingsMenu::HandleSliderUpdated(void *context, uint32_t id, float value)
{
    Everstead::GetInstance()->GetUiController()->PlayUiSound("ui.scroll");
    UIScene_SettingsMenu *_this = (UIScene_SettingsMenu *) context;
    Options *options            = &Everstead::GetInstance()->options;

    switch (id)
    {
        case s_settingsMenuAutosaveIntervalId: {
            int step                       = (int) round(value);
            options->autosaveIntervalTicks = AutosaveTicksForStep(step);
            Everstead::GetInstance()->ResetAutosaveTimer();
            _this->m_multiList->SetSliderText(id, FormatAutosaveText(step));
            break;
        }

        case s_settingsMenuDifficultyId: {
            options->difficulty = value;
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.game.difficulty"), value));
            break;
        }

        case s_settingsMenuMusicId: {
            options->musicVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.music"), value));
            break;
        }

        case s_settingsMenuSoundId: {
            options->soundVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.sound"), value));
            break;
        }

        case s_settingsMenuWeatherId: {
            options->weatherVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.weather"), value));
            break;
        }

        case s_settingsMenuBlocksId: {
            options->blockVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.blocks"), value));
            break;
        }

        case s_settingsMenuAmbientId: {
            options->ambientVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.ambient"), value));
            break;
        }

        case s_settingsMenuPlayersId: {
            options->playerVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.players"), value));
            break;
        }

        case s_settingsMenuUiSoundId: {
            options->uiVolume = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.ui"), value));
            break;
        }

        case s_settingsMenuSoundReverbId: {
            options->soundReverb = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.reverb"), value));
            break;
        }

        case s_settingsMenuSoundEchoId: {
            options->soundEcho = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.echo"), value));
            break;
        }

        case s_settingsMenuSoundMufflingId: {
            options->soundMuffling = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.audio.muffling"), value));
            break;
        }

        case s_settingsMenuGammaId: {
            options->gamma = value;
            Everstead::GetInstance()->GetGameRenderer()->SetGamma(value);
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.graphics.gamma"), value));
            break;
        }

        case s_settingsMenuRenderDistanceId: {
            Everstead::GetInstance()->SetRenderDistance((int) round(value));
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.render_distance"), value));
            break;
        }

        case s_settingsMenuHudSizeId: {
            options->hudSize = value;
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.ui.hud_size"), value));
            break;
        }

        case s_settingsMenuChatTextOpacityId: {
            options->chatTextOpacity = value;
            _this->m_multiList->SetSliderText(id,
                                              FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.ui.chat_text_opacity"), value));
            break;
        }

        case s_settingsMenuChatBackgroundId: {
            options->chatBackgroundOpacity = value;
            _this->m_multiList->SetSliderText(
                    id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.ui.chat_background_opacity"), value));
            break;
        }

        case s_settingsMenuChatTextSizeId: {
            options->chatTextSize = value;
            _this->m_multiList->SetSliderText(id,
                                              FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.ui.chat_text_size"), value));
            break;
        }

        case s_settingsMenuChatLineSpacingId: {
            options->chatLineSpacing = value;
            _this->m_multiList->SetSliderText(id,
                                              FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.ui.chat_line_spacing"), value));
            break;
        }

        case s_settingsMenuChatHeightId: {
            options->chatHeight = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.ui.chat_height"), value));
            break;
        }

        case s_settingsMenuChatWidthId: {
            options->chatWidth = value;
            _this->m_multiList->SetSliderText(id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.ui.chat_width"), value));
            break;
        }

        case s_settingsMenuNimbusQualityId: {
            options->nimbusQuality = (int) round(value);
            _this->m_multiList->SetSliderText(id,
                                              FormatChoiceText("options.graphics.nimbus_quality", s_nimbusQualityKeys, 3, options->nimbusQuality));
            break;
        }

        case s_settingsMenuRenderBackendId: {
            Everstead *everstead = Everstead::GetInstance();
            int index            = std::clamp((int) round(value), 0, RenderBackend::kBackendCount - 1);
            if (options->renderBackend != RenderBackend::kBackends[index].key)
            {
                options->renderBackend = RenderBackend::kBackends[index].key;
                everstead->GetUiController()->ShowNotification(everstead->GetString("options.graphics.render_backend_restart_title"),
                                                               everstead->GetString("options.graphics.render_backend_restart_message"), 3.0f);
            }

            _this->m_multiList->SetSliderText(id, everstead->GetString("options.graphics.render_backend") + L": " +
                                                          everstead->GetString(RenderBackend::kBackends[index].nameKey));
            break;
        }

        case s_settingsMenuFovId: {
            options->fov = value;
            Everstead::GetInstance()->GetGameRenderer()->SetFov(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.fov"), value));
            break;
        }

        case s_settingsMenuFpsDisplayId: {
            options->fpsDisplay = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatChoiceText("options.graphics.fps_display", s_fpsDisplayKeys, 3, options->fpsDisplay));
            break;
        }

        case s_settingsMenuSimulationDistanceId: {
            Everstead::GetInstance()->SetSimulationDistance((int) round(value));
            _this->m_multiList->SetSliderText(id,
                                              FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.simulation_distance"), value));
            break;
        }

        case s_settingsMenuEntityRenderDistanceId: {
            options->entityRenderDistance = (int) round(value);
            _this->m_multiList->SetSliderText(
                    id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.graphics.entity_render_distance"), value));
            break;
        }

        case s_settingsMenuCloudHeightId: {
            options->cloudHeight = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.cloud_height"), value));
            break;
        }

        case s_settingsMenuFogDistanceId: {
            options->fogDistance = value;
            _this->m_multiList->SetSliderText(
                    id, FormatSliderTextPercentage(Everstead::GetInstance()->GetString("options.graphics.fog_distance"), value));
            break;
        }

        case s_settingsMenuParticlesId: {
            options->particles = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatChoiceText("options.graphics.particles", s_particleKeys, 3, options->particles));
            break;
        }

        case s_settingsMenuParticleLimitId: {
            options->particleLimit = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.particle_limit"), value));
            break;
        }

        case s_settingsMenuDynamicLightsId: {
            options->dynamicLights = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatChoiceText("options.graphics.dynamic_lights", s_dynamicLightKeys, 3, options->dynamicLights));
            break;
        }

        case s_settingsMenuMipmapLevelsId: {
            options->mipmapLevels = (int) round(value);
            Everstead::GetInstance()->ApplyTextureOptions();
            _this->m_multiList->SetSliderText(id, FormatZeroOrValueText("options.graphics.mipmap_levels", "options.off", options->mipmapLevels, L""));
            break;
        }

        case s_settingsMenuAnisotropyId: {
            options->anisotropicFiltering = (int) round(value);
            Everstead::GetInstance()->ApplyTextureOptions();
            _this->m_multiList->SetSliderText(id, FormatZeroOrValueText("options.graphics.anisotropic_filtering", "options.off",
                                                                        options->anisotropicFiltering > 0 ? 1 << options->anisotropicFiltering : 0,
                                                                        L"x"));
            break;
        }

        case s_settingsMenuChunkUpdatesId: {
            options->chunkUploadsPerFrame = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.chunk_updates"), value));
            break;
        }

        case s_settingsMenuChunkThreadsId: {
            options->chunkWorkerThreads = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatZeroOrValueText("options.graphics.chunk_threads", "options.graphics.chunk_threads.auto",
                                                                        options->chunkWorkerThreads, L""));
            break;
        }

        case s_settingsMenuChunkUnloadId: {
            options->chunkUnloadMargin = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.chunk_unload"), value));
            break;
        }

        case s_settingsMenuChunkIntakeId: {
            options->chunkIntakePerTick = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.chunk_intake"), value));
            break;
        }

        case s_settingsMenuChunkSendsId: {
            options->chunkSendsPerTick = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.chunk_sends"), value));
            break;
        }

        case s_settingsMenuChunkInFlightId: {
            options->chunkSendsInFlight = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.chunk_in_flight"), value));
            break;
        }

        case s_settingsMenuChunkQueueId: {
            options->chunkQueuePerThread = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.chunk_queue"), value));
            break;
        }

        case s_settingsMenuLodDistanceId: {
            options->lodDistance = (int) round(value) * s_lodDistanceStep;
            _this->m_multiList->SetSliderText(
                    id, FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.lod_distance"), (float) options->lodDistance));
            break;
        }

        case s_settingsMenuLodQualityId: {
            options->lodQuality = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatChoiceText("options.graphics.lod_quality", s_lodQualityKeys, 4, options->lodQuality));
            break;
        }

        case s_settingsMenuLodCpuPresetId: {
            options->lodCpuPreset = (int) round(value);
            _this->m_multiList->SetSliderText(id, FormatChoiceText("options.graphics.lod_cpu_preset", s_lodCpuPresetKeys, 4, options->lodCpuPreset));
            break;
        }

        case s_settingsMenuLodTilesInFlightId: {
            options->lodTilesInFlight = (int) round(value);
            _this->m_multiList->SetSliderText(id,
                                              FormatSliderText(Everstead::GetInstance()->GetString("options.graphics.lod_tiles_in_flight"), value));
            break;
        }

        case s_settingsMenuLodMipmapLevelsId: {
            options->lodMipmapLevels = (int) round(value);
            _this->m_multiList->SetSliderText(
                    id, FormatZeroOrValueText("options.graphics.lod_mipmap_levels", "options.off", options->lodMipmapLevels, L""));
            break;
        }

        case s_settingsMenuLodFadeTimeId: {
            options->lodFadeTime = (int) round(value) * s_lodFadeTimeStep;
            _this->m_multiList->SetSliderText(id,
                                              FormatZeroOrValueText("options.graphics.lod_fade_time", "options.off", options->lodFadeTime, L" ms"));
            break;
        }

        case s_settingsMenuFpsLimitId: {
            int step          = (int) round(value);
            options->fpsLimit = step > s_fpsLimitSteps ? 0 : step * s_fpsLimitStep;
            _this->m_multiList->SetSliderText(id, FormatFpsLimitText(step));
            break;
        }

        default: {
            break;
        }
    }
}

void UIScene_SettingsMenu::BeginButtonPage(eCATEGORY category)
{
    m_currentCategory = category;
    m_multiList->ClearEntries();
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->SetPanelVisible(false);
    m_multiList->SetButtonTextures("assets/texture/ui/control/mainmenubutton.png", "assets/texture/ui/control/mainmenubutton_highlight.png");
    m_multiList->SetButtonRowWidth(600.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetRowSpacing(15.0f);
    m_multiList->SetContentPadding(s_multiListDefaultContentPaddingX, s_multiListDefaultContentPaddingY);
}

void UIScene_SettingsMenu::BeginOptionsPage(eCATEGORY category)
{
    m_currentCategory = category;
    m_multiList->ClearEntries();
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonTextures("assets/texture/ui/control/listbutton.png", "assets/texture/ui/control/listbutton_highlight.png");
    m_multiList->SetButtonRowWidth(0.0f);
    m_multiList->SetButtonRowHeight(50.0f);
    m_multiList->SetRowSpacing(6.0f);
}

void UIScene_SettingsMenu::FinishPage()
{
    m_multiList->ResizeHorizontal();
    m_multiList->ResizeVertical();
    m_multiList->SetPosition((Screen::WIDTH - m_multiList->GetWidth()) * 0.5f, (Screen::HEIGHT - m_multiList->GetHeight()) * 0.5f);
}

void UIScene_SettingsMenu::ShowRootCategories()
{
    Everstead *everstead = Everstead::GetInstance();
    BeginButtonPage(eCategory_None);

    m_multiList->AddNewButton(s_settingsMenuGameOptionsId, everstead->GetString("menu.settings.game_options"));
    m_multiList->AddNewButton(s_settingsMenuAudioId, everstead->GetString("menu.settings.audio"));
    m_multiList->AddNewButton(s_settingsMenuGraphicsId, everstead->GetString("menu.settings.graphics"));
    m_multiList->AddNewButton(s_settingsMenuUserInterfaceId, everstead->GetString("menu.settings.user_interface"));
    m_multiList->AddNewButton(s_settingsMenuResetToDefaultsId, everstead->GetString("menu.settings.reset_to_defaults"));
    FinishPage();

    if (m_rootSelectionIndex >= 0)
    {
        m_multiList->SetCurrentSelection(m_rootSelectionIndex);
    }
}

void UIScene_SettingsMenu::ShowGameOptions()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_GameOptions);

    m_multiList->AddNewCheckbox(s_settingsMenuViewBobbingId, everstead->GetString("options.game.view_bobbing"), options.viewBobbing);
    m_multiList->AddNewCheckbox(s_settingsMenuAnnounceSavesId, everstead->GetString("options.game.announce_world_saves"), options.announceWorldSaves);
    m_multiList->AddNewCheckbox(s_settingsMenuDiscordPresenceId, everstead->GetString("options.game.discord_rich_presence"),
                                options.discordRichPresence);
    m_multiList->AddNewButton(s_settingsMenuLanguagesId, everstead->GetString("menu.settings.languages"));
    m_multiList->AddNewSlider(s_settingsMenuAutosaveIntervalId, everstead->GetString("options.game.autosave_interval"), 0.0f, 4.0f,
                              (float) AutosaveStepForTicks(options.autosaveIntervalTicks), 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuDifficultyId, everstead->GetString("options.game.difficulty"), 0.0f, 3.0f, options.difficulty, 1.0f)
            ->SetVisible(false);

    m_multiList->SetSliderText(s_settingsMenuAutosaveIntervalId, FormatAutosaveText(AutosaveStepForTicks(options.autosaveIntervalTicks)));
    m_multiList->SetSliderText(s_settingsMenuDifficultyId, FormatSliderText(everstead->GetString("options.game.difficulty"),
                                                                            m_multiList->GetSliderValue(s_settingsMenuDifficultyId)));
    FinishPage();
}

void UIScene_SettingsMenu::ShowAudio()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_Audio);

    const uint32_t ids[]  = {s_settingsMenuSoundId,   s_settingsMenuMusicId,   s_settingsMenuWeatherId, s_settingsMenuBlocksId,
                             s_settingsMenuAmbientId, s_settingsMenuPlayersId, s_settingsMenuUiSoundId};
    const char *keys[]    = {"options.audio.sound",   "options.audio.music",   "options.audio.weather", "options.audio.blocks",
                             "options.audio.ambient", "options.audio.players", "options.audio.ui"};
    const float volumes[] = {options.soundVolume,   options.musicVolume,  options.weatherVolume, options.blockVolume,
                             options.ambientVolume, options.playerVolume, options.uiVolume};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++)
    {
        m_multiList->AddNewSlider(ids[i], everstead->GetString(keys[i]), 0.0f, 100.0f, volumes[i], 1.0f);
        m_multiList->SetSliderText(ids[i], FormatSliderTextPercentage(everstead->GetString(keys[i]), volumes[i]));
    }

    m_multiList->AddNewCheckbox(s_settingsMenuSoundRaytracingId, everstead->GetString("options.audio.raytracing"), options.soundRaytracing);

    const uint32_t acousticIds[] = {s_settingsMenuSoundReverbId, s_settingsMenuSoundEchoId, s_settingsMenuSoundMufflingId};
    const char *acousticKeys[]   = {"options.audio.reverb", "options.audio.echo", "options.audio.muffling"};
    const float acousticValues[] = {options.soundReverb, options.soundEcho, options.soundMuffling};
    for (size_t i = 0; i < sizeof(acousticIds) / sizeof(acousticIds[0]); i++)
    {
        m_multiList->AddNewSlider(acousticIds[i], everstead->GetString(acousticKeys[i]), 0.0f, 200.0f, acousticValues[i], 1.0f);
        m_multiList->SetSliderText(acousticIds[i], FormatSliderTextPercentage(everstead->GetString(acousticKeys[i]), acousticValues[i]));
    }
    FinishPage();
}

void UIScene_SettingsMenu::ShowGraphics()
{
    Everstead *everstead = Everstead::GetInstance();
    BeginButtonPage(eCategory_Graphics);

    m_multiList->AddNewButton(s_settingsMenuDisplayId, everstead->GetString("menu.settings.display"));
    m_multiList->AddNewButton(s_settingsMenuRenderingId, everstead->GetString("menu.settings.rendering"));
    m_multiList->AddNewButton(s_settingsMenuQualityId, everstead->GetString("menu.settings.quality"));
    m_multiList->AddNewButton(s_settingsMenuLodId, everstead->GetString("menu.settings.lod"));
    if (RenderDevice::GetInstance()->GetBackend() != eRenderBackend_OpenGL)
    {
        m_multiList->AddNewButton(s_settingsMenuNimbusId, everstead->GetString("menu.settings.nimbus"));
    }

    m_multiList->AddNewButton(s_settingsMenuPerformanceId, everstead->GetString("menu.settings.performance"));
    FinishPage();

    if (m_subSelectionIndex >= 0)
    {
        m_multiList->SetCurrentSelection(m_subSelectionIndex);
    }
}

void UIScene_SettingsMenu::ShowGraphicsDisplay()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    int fpsStep          = options.fpsLimit <= 0 ? s_fpsLimitSteps + 1 : std::clamp(options.fpsLimit / s_fpsLimitStep, 1, s_fpsLimitSteps);
    BeginOptionsPage(eCategory_GraphicsDisplay);

    m_multiList->AddNewButton(s_settingsMenuWindowModeId, FormatChoiceText("options.graphics.window_mode", s_windowModeKeys, 3, options.windowMode));
    m_multiList->AddNewSlider(s_settingsMenuFpsLimitId, everstead->GetString("options.graphics.fps_limit"), 1.0f, (float) (s_fpsLimitSteps + 1),
                              (float) fpsStep, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuVsyncId, everstead->GetString("options.graphics.vertical_sync"), options.vsync);
    m_multiList->AddNewSlider(s_settingsMenuFovId, everstead->GetString("options.graphics.fov"), 30.0f, 110.0f, options.fov, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuGammaId, everstead->GetString("options.graphics.gamma"), 0.0f, 100.0f, options.gamma, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuFpsDisplayId, everstead->GetString("options.graphics.fps_display"), (float) eFpsDisplay_Off,
                              (float) eFpsDisplay_Extended, (float) options.fpsDisplay, 1.0f);

    m_multiList->SetSliderText(s_settingsMenuFpsLimitId, FormatFpsLimitText(fpsStep));
    m_multiList->SetSliderText(s_settingsMenuFovId, FormatSliderText(everstead->GetString("options.graphics.fov"), options.fov));
    m_multiList->SetSliderText(s_settingsMenuGammaId, FormatSliderTextPercentage(everstead->GetString("options.graphics.gamma"), options.gamma));
    m_multiList->SetSliderText(s_settingsMenuFpsDisplayId, FormatChoiceText("options.graphics.fps_display", s_fpsDisplayKeys, 3, options.fpsDisplay));

    if (!RenderBackend::kForced)
    {
        int backendIndex = RenderBackend::FindIndex(options.renderBackend);
        m_multiList->AddNewSlider(s_settingsMenuRenderBackendId, everstead->GetString("options.graphics.render_backend"), 0.0f,
                                  (float) (RenderBackend::kBackendCount - 1), (float) backendIndex, 1.0f);
        m_multiList->SetSliderText(s_settingsMenuRenderBackendId, everstead->GetString("options.graphics.render_backend") + L": " +
                                                                          everstead->GetString(RenderBackend::kBackends[backendIndex].nameKey));
    }

    FinishPage();
}

void UIScene_SettingsMenu::ShowGraphicsRendering()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_GraphicsRendering);

    m_multiList->AddNewSlider(s_settingsMenuRenderDistanceId, everstead->GetString("options.graphics.render_distance"), 2.0f, 128.0f,
                              options.renderDistance, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuSimulationDistanceId, everstead->GetString("options.graphics.simulation_distance"), 2.0f, 32.0f,
                              (float) options.simulationDistance, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuEntityRenderDistanceId, everstead->GetString("options.graphics.entity_render_distance"), 10.0f, 100.0f,
                              (float) options.entityRenderDistance, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuRenderCloudsId, everstead->GetString("options.graphics.render_clouds"), options.renderClouds);
    m_multiList->AddNewSlider(s_settingsMenuCloudHeightId, everstead->GetString("options.graphics.cloud_height"), 64.0f, 320.0f,
                              (float) options.cloudHeight, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuEntityShadowsId, everstead->GetString("options.graphics.entity_shadows"), options.entityShadows);
    m_multiList->AddNewCheckbox(s_settingsMenuSmartLeavesId, everstead->GetString("options.graphics.smart_leaves"), options.smartLeaves);
    m_multiList->AddNewCheckbox(s_settingsMenuFlatItemsId, everstead->GetString("options.graphics.flat_items"), options.flatItems);
    m_multiList->AddNewSlider(s_settingsMenuFogDistanceId, everstead->GetString("options.graphics.fog_distance"), 10.0f, 100.0f, options.fogDistance,
                              1.0f);
    m_multiList->AddNewSlider(s_settingsMenuParticlesId, everstead->GetString("options.graphics.particles"), (float) eParticleSetting_All,
                              (float) eParticleSetting_Minimal, (float) options.particles, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuParticleLimitId, everstead->GetString("options.graphics.particle_limit"), 500.0f, 16000.0f,
                              (float) options.particleLimit, 500.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuSmoothLightingId, everstead->GetString("options.graphics.smooth_lighting"), options.smoothLighting);
    m_multiList->AddNewCheckbox(s_settingsMenuAmbientOcclusionId, everstead->GetString("options.graphics.ambient_occlusion"),
                                options.ambientOcclusion);
    m_multiList->AddNewSlider(s_settingsMenuDynamicLightsId, everstead->GetString("options.graphics.dynamic_lights"),
                              (float) eDynamicLightSetting_Off, (float) eDynamicLightSetting_Fancy, (float) options.dynamicLights, 1.0f);

    m_multiList->SetSliderText(s_settingsMenuRenderDistanceId,
                               FormatSliderText(everstead->GetString("options.graphics.render_distance"), options.renderDistance));
    m_multiList->SetSliderText(s_settingsMenuSimulationDistanceId,
                               FormatSliderText(everstead->GetString("options.graphics.simulation_distance"), (float) options.simulationDistance));
    m_multiList->SetSliderText(
            s_settingsMenuEntityRenderDistanceId,
            FormatSliderTextPercentage(everstead->GetString("options.graphics.entity_render_distance"), (float) options.entityRenderDistance));
    m_multiList->SetSliderText(s_settingsMenuCloudHeightId,
                               FormatSliderText(everstead->GetString("options.graphics.cloud_height"), (float) options.cloudHeight));
    m_multiList->SetSliderText(s_settingsMenuFogDistanceId,
                               FormatSliderTextPercentage(everstead->GetString("options.graphics.fog_distance"), options.fogDistance));
    m_multiList->SetSliderText(s_settingsMenuParticlesId, FormatChoiceText("options.graphics.particles", s_particleKeys, 3, options.particles));
    m_multiList->SetSliderText(s_settingsMenuParticleLimitId,
                               FormatSliderText(everstead->GetString("options.graphics.particle_limit"), (float) options.particleLimit));
    m_multiList->SetSliderText(s_settingsMenuDynamicLightsId,
                               FormatChoiceText("options.graphics.dynamic_lights", s_dynamicLightKeys, 3, options.dynamicLights));
    FinishPage();
}

void UIScene_SettingsMenu::ShowGraphicsQuality()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_GraphicsQuality);

    m_multiList->AddNewSlider(s_settingsMenuMipmapLevelsId, everstead->GetString("options.graphics.mipmap_levels"), 0.0f,
                              (float) (TextureAtlas::s_mipLevelCount - 1), (float) options.mipmapLevels, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuAnisotropyId, everstead->GetString("options.graphics.anisotropic_filtering"), 0.0f,
                              (float) s_maxAnisotropyStep, (float) options.anisotropicFiltering, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuAnimatedTexturesId, everstead->GetString("options.graphics.animated_textures"),
                                options.animatedTextures);

    m_multiList->SetSliderText(s_settingsMenuMipmapLevelsId,
                               FormatZeroOrValueText("options.graphics.mipmap_levels", "options.off", options.mipmapLevels, L""));
    m_multiList->SetSliderText(s_settingsMenuAnisotropyId,
                               FormatZeroOrValueText("options.graphics.anisotropic_filtering", "options.off",
                                                     options.anisotropicFiltering > 0 ? 1 << options.anisotropicFiltering : 0, L"x"));
    FinishPage();
}

void UIScene_SettingsMenu::ShowGraphicsNimbus()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_GraphicsNimbus);

    m_multiList->AddNewCheckbox(s_settingsMenuNimbusEnabledId, everstead->GetString("options.graphics.nimbus_enabled"), options.nimbusEnabled);
    m_multiList->AddNewCheckbox(s_settingsMenuNimbusLightingId, everstead->GetString("options.graphics.nimbus_lighting"), options.nimbusLighting);
    m_multiList->AddNewCheckbox(s_settingsMenuNimbusCloudsId, everstead->GetString("options.graphics.nimbus_clouds"), options.nimbusClouds);
    m_multiList->AddNewCheckbox(s_settingsMenuNimbusFogId, everstead->GetString("options.graphics.nimbus_fog"), options.nimbusFog);
    m_multiList->AddNewSlider(s_settingsMenuNimbusQualityId, everstead->GetString("options.graphics.nimbus_quality"), 0.0f, 2.0f,
                              (float) options.nimbusQuality, 1.0f);

    m_multiList->SetSliderText(s_settingsMenuNimbusQualityId,
                               FormatChoiceText("options.graphics.nimbus_quality", s_nimbusQualityKeys, 3, options.nimbusQuality));
    FinishPage();
}

void UIScene_SettingsMenu::ShowGraphicsLod()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_GraphicsLod);

    m_multiList->AddNewCheckbox(s_settingsMenuLodEnabledId, everstead->GetString("options.graphics.lod_enabled"), options.lodEnabled);
    m_multiList->AddNewSlider(s_settingsMenuLodDistanceId, everstead->GetString("options.graphics.lod_distance"), 2.0f, (float) s_maxLodDistanceSteps,
                              (float) (options.lodDistance / s_lodDistanceStep), 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuLodQualityId, everstead->GetString("options.graphics.lod_quality"), 0.0f, 3.0f,
                              (float) options.lodQuality, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuLodGenerateTerrainId, everstead->GetString("options.graphics.lod_generate_terrain"),
                                options.lodGenerateTerrain);
    m_multiList->AddNewSlider(s_settingsMenuLodCpuPresetId, everstead->GetString("options.graphics.lod_cpu_preset"), 0.0f, 3.0f,
                              (float) options.lodCpuPreset, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuLodTilesInFlightId, everstead->GetString("options.graphics.lod_tiles_in_flight"), 1.0f,
                              (float) s_maxLodTilesInFlight, (float) options.lodTilesInFlight, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuLodFadeTimeId, everstead->GetString("options.graphics.lod_fade_time"), 0.0f,
                              (float) s_maxLodFadeTimeSteps, (float) (options.lodFadeTime / s_lodFadeTimeStep), 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuLodTexturesId, everstead->GetString("options.graphics.lod_textures"), options.lodTextures);
    m_multiList->AddNewSlider(s_settingsMenuLodMipmapLevelsId, everstead->GetString("options.graphics.lod_mipmap_levels"), 0.0f,
                              (float) LodRenderer::s_maxMipmapLevels, (float) options.lodMipmapLevels, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuLodDebugId, everstead->GetString("options.graphics.lod_debug"), options.lodDebug);

    m_multiList->SetSliderText(s_settingsMenuLodDistanceId,
                               FormatSliderText(everstead->GetString("options.graphics.lod_distance"), (float) options.lodDistance));
    m_multiList->SetSliderText(s_settingsMenuLodQualityId, FormatChoiceText("options.graphics.lod_quality", s_lodQualityKeys, 4, options.lodQuality));
    m_multiList->SetSliderText(s_settingsMenuLodCpuPresetId,
                               FormatChoiceText("options.graphics.lod_cpu_preset", s_lodCpuPresetKeys, 4, options.lodCpuPreset));
    m_multiList->SetSliderText(s_settingsMenuLodTilesInFlightId,
                               FormatSliderText(everstead->GetString("options.graphics.lod_tiles_in_flight"), (float) options.lodTilesInFlight));
    m_multiList->SetSliderText(s_settingsMenuLodMipmapLevelsId,
                               FormatZeroOrValueText("options.graphics.lod_mipmap_levels", "options.off", options.lodMipmapLevels, L""));
    m_multiList->SetSliderText(s_settingsMenuLodFadeTimeId,
                               FormatZeroOrValueText("options.graphics.lod_fade_time", "options.off", options.lodFadeTime, L" ms"));
    FinishPage();
}

void UIScene_SettingsMenu::ShowGraphicsPerformance()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_GraphicsPerformance);

    m_multiList->AddNewCheckbox(s_settingsMenuOcclusionCullingId, everstead->GetString("options.graphics.occlusion_culling"),
                                options.occlusionCulling);
    m_multiList->AddNewCheckbox(s_settingsMenuFogCullingId, everstead->GetString("options.graphics.fog_culling"), options.fogCulling);
    m_multiList->AddNewCheckbox(s_settingsMenuFrustumCullingId, everstead->GetString("options.graphics.frustum_culling"), options.frustumCulling);
    m_multiList->AddNewCheckbox(s_settingsMenuGpuChunkCullingId, everstead->GetString("options.graphics.gpu_chunk_culling"), options.gpuChunkCulling);
    m_multiList->AddNewSlider(s_settingsMenuChunkUpdatesId, everstead->GetString("options.graphics.chunk_updates"), 1.0f, (float) s_maxChunkUpdates,
                              (float) options.chunkUploadsPerFrame, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChunkThreadsId, everstead->GetString("options.graphics.chunk_threads"), 0.0f,
                              (float) std::thread::hardware_concurrency(), (float) options.chunkWorkerThreads, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChunkUnloadId, everstead->GetString("options.graphics.chunk_unload"), 0.0f,
                              (float) s_maxChunkUnloadMargin, (float) options.chunkUnloadMargin, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChunkIntakeId, everstead->GetString("options.graphics.chunk_intake"), 1.0f, (float) s_maxChunkStreaming,
                              (float) options.chunkIntakePerTick, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChunkSendsId, everstead->GetString("options.graphics.chunk_sends"), 1.0f, (float) s_maxChunkStreaming,
                              (float) options.chunkSendsPerTick, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChunkInFlightId, everstead->GetString("options.graphics.chunk_in_flight"), 1.0f,
                              (float) s_maxChunkStreaming, (float) options.chunkSendsInFlight, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChunkQueueId, everstead->GetString("options.graphics.chunk_queue"), 1.0f,
                              (float) s_maxChunkQueuePerThread, (float) options.chunkQueuePerThread, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuSyncBlockUpdatesId, everstead->GetString("options.graphics.sync_block_updates"),
                                options.synchronizedBlockUpdates);
    m_multiList->AddNewCheckbox(s_settingsMenuInstantBlockUpdatesId, everstead->GetString("options.graphics.instant_block_updates"),
                                options.instantBlockUpdates);

    m_multiList->SetSliderText(s_settingsMenuChunkUpdatesId,
                               FormatSliderText(everstead->GetString("options.graphics.chunk_updates"), (float) options.chunkUploadsPerFrame));
    m_multiList->SetSliderText(
            s_settingsMenuChunkThreadsId,
            FormatZeroOrValueText("options.graphics.chunk_threads", "options.graphics.chunk_threads.auto", options.chunkWorkerThreads, L""));
    m_multiList->SetSliderText(s_settingsMenuChunkUnloadId,
                               FormatSliderText(everstead->GetString("options.graphics.chunk_unload"), (float) options.chunkUnloadMargin));
    m_multiList->SetSliderText(s_settingsMenuChunkIntakeId,
                               FormatSliderText(everstead->GetString("options.graphics.chunk_intake"), (float) options.chunkIntakePerTick));
    m_multiList->SetSliderText(s_settingsMenuChunkSendsId,
                               FormatSliderText(everstead->GetString("options.graphics.chunk_sends"), (float) options.chunkSendsPerTick));
    m_multiList->SetSliderText(s_settingsMenuChunkInFlightId,
                               FormatSliderText(everstead->GetString("options.graphics.chunk_in_flight"), (float) options.chunkSendsInFlight));
    m_multiList->SetSliderText(s_settingsMenuChunkQueueId,
                               FormatSliderText(everstead->GetString("options.graphics.chunk_queue"), (float) options.chunkQueuePerThread));
    FinishPage();
}

void UIScene_SettingsMenu::ShowUserInterface()
{
    Everstead *everstead = Everstead::GetInstance();
    BeginButtonPage(eCategory_UserInterface);

    m_multiList->AddNewButton(s_settingsMenuHudId, everstead->GetString("menu.settings.hud"));
    m_multiList->AddNewButton(s_settingsMenuChatId, everstead->GetString("menu.settings.chat"));
    FinishPage();

    if (m_subSelectionIndex >= 0)
    {
        m_multiList->SetCurrentSelection(m_subSelectionIndex);
    }
}

void UIScene_SettingsMenu::ShowUserInterfaceHud()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_UserInterfaceHud);

    m_multiList->AddNewCheckbox(s_settingsMenuDisplayHudId, everstead->GetString("options.ui.display_hud"), options.displayHud);
    m_multiList->AddNewCheckbox(s_settingsMenuDisplayHandId, everstead->GetString("options.ui.display_hand"), options.displayHand);
    m_multiList->AddNewSlider(s_settingsMenuHudSizeId, everstead->GetString("options.ui.hud_size"), 1.0f, 3.0f, options.hudSize, 1.0f);
    m_multiList->AddNewCheckbox(s_settingsMenuJavaTooltipsId, everstead->GetString("options.ui.java_tooltips"), options.javaTooltips);
    m_multiList->SetSliderText(s_settingsMenuHudSizeId, FormatSliderText(everstead->GetString("options.ui.hud_size"), options.hudSize));
    FinishPage();
}

void UIScene_SettingsMenu::ShowUserInterfaceChat()
{
    Everstead *everstead = Everstead::GetInstance();
    Options options      = everstead->options;
    BeginOptionsPage(eCategory_UserInterfaceChat);

    m_multiList->AddNewCheckbox(s_settingsMenuChatShownId, everstead->GetString("options.ui.chat_shown"), options.chatShown);
    m_multiList->AddNewSlider(s_settingsMenuChatTextOpacityId, everstead->GetString("options.ui.chat_text_opacity"), 0.0f, 100.0f,
                              options.chatTextOpacity, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChatBackgroundId, everstead->GetString("options.ui.chat_background_opacity"), 0.0f, 100.0f,
                              options.chatBackgroundOpacity, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChatTextSizeId, everstead->GetString("options.ui.chat_text_size"), 50.0f, 200.0f, options.chatTextSize,
                              1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChatLineSpacingId, everstead->GetString("options.ui.chat_line_spacing"), 0.0f, 100.0f,
                              options.chatLineSpacing, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChatHeightId, everstead->GetString("options.ui.chat_height"), 10.0f, 100.0f, options.chatHeight, 1.0f);
    m_multiList->AddNewSlider(s_settingsMenuChatWidthId, everstead->GetString("options.ui.chat_width"), 10.0f, 100.0f, options.chatWidth, 1.0f);

    m_multiList->SetSliderText(s_settingsMenuChatTextOpacityId,
                               FormatSliderTextPercentage(everstead->GetString("options.ui.chat_text_opacity"), options.chatTextOpacity));
    m_multiList->SetSliderText(s_settingsMenuChatBackgroundId,
                               FormatSliderTextPercentage(everstead->GetString("options.ui.chat_background_opacity"), options.chatBackgroundOpacity));
    m_multiList->SetSliderText(s_settingsMenuChatTextSizeId,
                               FormatSliderTextPercentage(everstead->GetString("options.ui.chat_text_size"), options.chatTextSize));
    m_multiList->SetSliderText(s_settingsMenuChatLineSpacingId,
                               FormatSliderTextPercentage(everstead->GetString("options.ui.chat_line_spacing"), options.chatLineSpacing));
    m_multiList->SetSliderText(s_settingsMenuChatHeightId,
                               FormatSliderTextPercentage(everstead->GetString("options.ui.chat_height"), options.chatHeight));
    m_multiList->SetSliderText(s_settingsMenuChatWidthId,
                               FormatSliderTextPercentage(everstead->GetString("options.ui.chat_width"), options.chatWidth));
    FinishPage();
}

void UIScene_SettingsMenu::ShowResetToDefaults()
{
    Everstead *everstead = Everstead::GetInstance();
    m_currentCategory    = eCategory_ResetToDefaults;

    m_multiList->ClearEntries();
    m_multiList->SetSize(0.0f, 0.0f);
    m_multiList->SetPanelVisible(true);
    m_multiList->SetButtonRowWidth(480.0f);
    m_multiList->SetRowSpacing(6.0f);
    m_multiList->SetLabelRowWidth(480.0f);
    m_multiList->SetContentPadding(40.0f, 30.0f);

    m_multiList->AddNewLabel(0, everstead->GetString("menu.settings.reset_settings") + L"\n");
    UIControl_Label *confirmLabel = m_multiList->AddNewLabel(0, everstead->GetString("menu.settings.reset_confirm") + L"\n");
    confirmLabel->SetWrapMode(UIControl_Label::eWrapMode_Word);
    m_multiList->AddNewButton(s_settingsMenuResetCancelId, everstead->GetString("button.cancel"));
    m_multiList->AddNewButton(s_settingsMenuResetOkId, everstead->GetString("button.ok"));
    FinishPage();
}

#endif
