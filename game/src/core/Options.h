#pragma once

#include <cstdint>
#include <string>

#include "../input/ControllerBinding.h"
#include "../input/KeyBinding.h"

enum ePARTICLE_SETTING : int
{
    eParticleSetting_All       = 0,
    eParticleSetting_Decreased = 1,
    eParticleSetting_Minimal   = 2,
};

enum eDYNAMIC_LIGHT_SETTING : int
{
    eDynamicLightSetting_Off   = 0,
    eDynamicLightSetting_Fast  = 1,
    eDynamicLightSetting_Fancy = 2,
};

enum eFPS_DISPLAY : int
{
    eFpsDisplay_Off      = 0,
    eFpsDisplay_Basic    = 1,
    eFpsDisplay_Extended = 2,
};

class Options
{
public:
    void Reset();
    void Load();
    void Save() const;
    float GetCategoryVolume(uint8_t category) const;
    float GetHudScale() const;

    float gamma;
    float renderDistance;
    bool viewBobbing;
    bool announceWorldSaves;
    bool discordRichPresence;
    int8_t language;
    int autosaveIntervalTicks;
    float mouseSensitivity;
    float controllerSensitivity;
    float menuCursorSensitivity;
    float difficulty;
    float musicVolume;
    float soundVolume;
    float weatherVolume;
    float blockVolume;
    float ambientVolume;
    float playerVolume;
    float uiVolume;
    bool soundRaytracing;
    float soundReverb;
    float soundEcho;
    float soundMuffling;
    bool vsync;
    int windowMode;
    std::string renderBackend;
    int fpsLimit;
    bool renderClouds;
    int cloudHeight;
    bool entityShadows;
    bool smartLeaves;
    bool flatItems;
    bool occlusionCulling;
    bool fogCulling;
    bool frustumCulling;
    bool gpuChunkCulling;
    bool smoothLighting;
    bool ambientOcclusion;
    bool nimbusEnabled;
    bool nimbusLighting;
    bool nimbusClouds;
    bool nimbusFog;
    int nimbusQuality;
    int dynamicLights;
    bool animatedTextures;
    float fogDistance;
    float fov;
    int simulationDistance;
    int entityRenderDistance;
    int particles;
    int particleLimit;
    int mipmapLevels;
    int anisotropicFiltering;
    int chunkUploadsPerFrame;
    int chunkWorkerThreads;
    int chunkUnloadMargin;
    int chunkIntakePerTick;
    int chunkSendsPerTick;
    int chunkSendsInFlight;
    int chunkQueuePerThread;
    bool lodEnabled;
    int lodDistance;
    int lodQuality;
    int lodCpuPreset;
    bool lodGenerateTerrain;
    int lodTilesInFlight;
    int lodFadeTime;
    bool lodTextures;
    int lodMipmapLevels;
    bool lodDebug;
    bool synchronizedBlockUpdates;
    bool instantBlockUpdates;
    int fpsDisplay;
    bool onlineGame;
    bool allowFriendsOfFriends;
    int maxPlayers;
    bool displayHud;
    bool displayHand;
    float hudSize;
    bool javaTooltips;
    bool advancedTooltips;
    bool chatShown;
    float chatTextOpacity;
    float chatBackgroundOpacity;
    float chatTextSize;
    float chatLineSpacing;
    float chatHeight;
    float chatWidth;
    bool invertLook;
    bool keyboardToggleSprint;
    bool keyboardToggleSneak;
    bool controllerToggleSprint;
    bool controllerToggleSneak;
    int keyBindings[g_keyBindingCount];
    int controllerBindings[g_controllerBindingCount];

private:
    static const int s_legacyBindingCount = 15;
    static const int s_legacyBindings[s_legacyBindingCount];

    void ApplyValue(const std::string &key, const std::string &value);
};
