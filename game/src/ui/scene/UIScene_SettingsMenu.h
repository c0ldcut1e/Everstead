#pragma once

#include <cstdint>
#include <string>

#include "../component/UIComponent_Fill.h"
#include "../control/UIControl_MultiList.h"
#include "UIScene.h"

class UIScene_SettingsMenu : public UIScene
{
public:
    enum eCATEGORY : uint8_t
    {
        eCategory_None                = 0,
        eCategory_GameOptions         = 1,
        eCategory_Audio               = 2,
        eCategory_Graphics            = 3,
        eCategory_UserInterface       = 4,
        eCategory_ResetToDefaults     = 5,
        eCategory_GraphicsDisplay     = 6,
        eCategory_GraphicsRendering   = 7,
        eCategory_GraphicsPerformance = 8,
        eCategory_UserInterfaceHud    = 9,
        eCategory_UserInterfaceChat   = 10,
        eCategory_GraphicsQuality     = 11,
        eCategory_GraphicsLod         = 12,
        eCategory_GraphicsNimbus      = 13,
    };

    UIScene_SettingsMenu();
    ~UIScene_SettingsMenu() override;

    void OnEnter() override;
    void OnExit() override;
    void OnGameplayAction(eGAMEPLAY_ACTION action) override;

private:
    static const uint32_t s_settingsMenuGameOptionsId          = 1;
    static const uint32_t s_settingsMenuAudioId                = 2;
    static const uint32_t s_settingsMenuGraphicsId             = 3;
    static const uint32_t s_settingsMenuUserInterfaceId        = 4;
    static const uint32_t s_settingsMenuResetToDefaultsId      = 5;
    static const uint32_t s_settingsMenuDisplayId              = 6;
    static const uint32_t s_settingsMenuRenderingId            = 7;
    static const uint32_t s_settingsMenuPerformanceId          = 8;
    static const uint32_t s_settingsMenuHudId                  = 9;
    static const uint32_t s_settingsMenuChatId                 = 10;
    static const uint32_t s_settingsMenuQualityId              = 11;
    static const uint32_t s_settingsMenuLodId                  = 12;
    static const uint32_t s_settingsMenuNimbusId               = 13;
    static const uint32_t s_settingsMenuViewBobbingId          = 101;
    static const uint32_t s_settingsMenuLanguagesId            = 103;
    static const uint32_t s_settingsMenuAutosaveIntervalId     = 104;
    static const uint32_t s_settingsMenuAnnounceSavesId        = 105;
    static const uint32_t s_settingsMenuDifficultyId           = 106;
    static const uint32_t s_settingsMenuDiscordPresenceId      = 107;
    static const uint32_t s_settingsMenuMusicId                = 200;
    static const uint32_t s_settingsMenuSoundId                = 201;
    static const uint32_t s_settingsMenuWeatherId              = 202;
    static const uint32_t s_settingsMenuBlocksId               = 203;
    static const uint32_t s_settingsMenuAmbientId              = 204;
    static const uint32_t s_settingsMenuPlayersId              = 205;
    static const uint32_t s_settingsMenuUiSoundId              = 206;
    static const uint32_t s_settingsMenuSoundRaytracingId      = 207;
    static const uint32_t s_settingsMenuSoundReverbId          = 208;
    static const uint32_t s_settingsMenuSoundEchoId            = 209;
    static const uint32_t s_settingsMenuSoundMufflingId        = 210;
    static const uint32_t s_settingsMenuVsyncId                = 300;
    static const uint32_t s_settingsMenuRenderCloudsId         = 302;
    static const uint32_t s_settingsMenuCloudHeightId          = 348;
    static const uint32_t s_settingsMenuOcclusionCullingId     = 303;
    static const uint32_t s_settingsMenuRenderDistanceId       = 304;
    static const uint32_t s_settingsMenuGammaId                = 305;
    static const uint32_t s_settingsMenuWindowModeId           = 306;
    static const uint32_t s_settingsMenuFpsLimitId             = 307;
    static const uint32_t s_settingsMenuFrustumCullingId       = 308;
    static const uint32_t s_settingsMenuGpuChunkCullingId      = 311;
    static const uint32_t s_settingsMenuFovId                  = 309;
    static const uint32_t s_settingsMenuFpsDisplayId           = 310;
    static const uint32_t s_settingsMenuFogDistanceId          = 313;
    static const uint32_t s_settingsMenuParticlesId            = 314;
    static const uint32_t s_settingsMenuParticleLimitId        = 354;
    static const uint32_t s_settingsMenuSmoothLightingId       = 315;
    static const uint32_t s_settingsMenuAmbientOcclusionId     = 316;
    static const uint32_t s_settingsMenuDynamicLightsId        = 317;
    static const uint32_t s_settingsMenuNimbusEnabledId        = 351;
    static const uint32_t s_settingsMenuNimbusLightingId       = 349;
    static const uint32_t s_settingsMenuNimbusCloudsId         = 352;
    static const uint32_t s_settingsMenuNimbusFogId            = 353;
    static const uint32_t s_settingsMenuNimbusQualityId        = 350;
    static const uint32_t s_settingsMenuMipmapLevelsId         = 318;
    static const uint32_t s_settingsMenuAnisotropyId           = 319;
    static const uint32_t s_settingsMenuAnimatedTexturesId     = 320;
    static const uint32_t s_settingsMenuChunkUpdatesId         = 321;
    static const uint32_t s_settingsMenuChunkThreadsId         = 322;
    static const uint32_t s_settingsMenuChunkUnloadId          = 323;
    static const uint32_t s_settingsMenuSyncBlockUpdatesId     = 324;
    static const uint32_t s_settingsMenuInstantBlockUpdatesId  = 325;
    static const uint32_t s_settingsMenuSimulationDistanceId   = 326;
    static const uint32_t s_settingsMenuFogCullingId           = 327;
    static const uint32_t s_settingsMenuRenderBackendId        = 328;
    static const uint32_t s_settingsMenuChunkIntakeId          = 329;
    static const uint32_t s_settingsMenuChunkSendsId           = 330;
    static const uint32_t s_settingsMenuChunkInFlightId        = 331;
    static const uint32_t s_settingsMenuChunkQueueId           = 332;
    static const uint32_t s_settingsMenuEntityShadowsId        = 333;
    static const uint32_t s_settingsMenuSmartLeavesId          = 334;
    static const uint32_t s_settingsMenuFlatItemsId            = 335;
    static const uint32_t s_settingsMenuEntityRenderDistanceId = 336;
    static const uint32_t s_settingsMenuLodEnabledId           = 337;
    static const uint32_t s_settingsMenuLodDistanceId          = 338;
    static const uint32_t s_settingsMenuLodQualityId           = 339;
    static const uint32_t s_settingsMenuLodCpuPresetId         = 340;
    static const uint32_t s_settingsMenuLodTilesInFlightId     = 341;
    static const uint32_t s_settingsMenuLodFadeTimeId          = 342;
    static const uint32_t s_settingsMenuLodDebugId             = 343;
    static const uint32_t s_settingsMenuLodTexturesId          = 344;
    static const uint32_t s_settingsMenuLodMipmapLevelsId      = 345;
    static const uint32_t s_settingsMenuLodGenerateTerrainId   = 347;
    static const uint32_t s_settingsMenuDisplayHudId           = 500;
    static const uint32_t s_settingsMenuHudSizeId              = 501;
    static const uint32_t s_settingsMenuDisplayHandId          = 502;
    static const uint32_t s_settingsMenuChatShownId            = 503;
    static const uint32_t s_settingsMenuChatTextOpacityId      = 504;
    static const uint32_t s_settingsMenuChatBackgroundId       = 505;
    static const uint32_t s_settingsMenuChatTextSizeId         = 506;
    static const uint32_t s_settingsMenuChatLineSpacingId      = 507;
    static const uint32_t s_settingsMenuChatHeightId           = 510;
    static const uint32_t s_settingsMenuChatWidthId            = 512;
    static const uint32_t s_settingsMenuJavaTooltipsId         = 509;
    static const uint32_t s_settingsMenuResetCancelId          = 400;
    static const uint32_t s_settingsMenuResetOkId              = 401;
    static const int s_fpsLimitStep                            = 5;
    static inline const int s_fpsLimitSteps                    = 72;
    static const int s_maxAnisotropyStep                       = 4;
    static const int s_maxChunkUpdates                         = 64;
    static const int s_maxChunkUnloadMargin                    = 8;
    static const int s_maxChunkStreaming                       = 64;
    static const int s_maxChunkQueuePerThread                  = 16;
    static const int s_lodDistanceStep                         = 16;
    static const int s_maxLodDistanceSteps                     = 32;
    static const int s_maxLodTilesInFlight                     = 256;
    static const int s_lodFadeTimeStep                         = 100;
    static const int s_maxLodFadeTimeSteps                     = 20;

    static void HandleButtonPressed(void *context, uint32_t id);
    static void HandleCheckboxUpdated(void *context, uint32_t id, bool checked);
    static void HandleSliderUpdated(void *context, uint32_t id, float value);

    void BeginButtonPage(eCATEGORY category);
    void BeginOptionsPage(eCATEGORY category);
    void FinishPage();

    void ShowRootCategories();
    void ShowGameOptions();
    void ShowAudio();
    void ShowGraphics();
    void ShowGraphicsDisplay();
    void ShowGraphicsRendering();
    void ShowGraphicsPerformance();
    void ShowGraphicsQuality();
    void ShowGraphicsLod();
    void ShowGraphicsNimbus();
    void ShowUserInterface();
    void ShowUserInterfaceHud();
    void ShowUserInterfaceChat();
    void ShowResetToDefaults();

    static std::wstring FormatSliderText(const std::wstring &name, float value);
    static std::wstring FormatSliderTextPercentage(const std::wstring &name, float value);
    static int AutosaveTicksForStep(int step);
    static int AutosaveStepForTicks(int ticks);
    static std::wstring FormatAutosaveText(int step);
    static std::wstring FormatChoiceText(const char *nameKey, const char *const *choiceKeys, int count, int index);
    static std::wstring FormatZeroOrValueText(const char *nameKey, const char *zeroKey, int value, const std::wstring &suffix);
    static std::wstring FormatFpsLimitText(int step);

    UIGroup *m_groupRoot;
    UIComponent_Fill *m_backgroundFill;
    UIControl_MultiList *m_multiList;
    eCATEGORY m_currentCategory;
    int m_rootSelectionIndex;
    int m_subSelectionIndex;
};
