#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Options.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "../input/InputManager.h"
#include "../io/File.h"
#include "../io/FileSystem.h"
#include "../rendering/RenderBackend.h"
#include "../utils/StringUtils.h"
#include "../world/lod/LodRenderer.h"
#include "Logger.h"
#include "Window.h"

const int Options::s_legacyBindings[s_legacyBindingCount] = {
        eKeyBinding_Forward,     eKeyBinding_Back,       eKeyBinding_Left,   eKeyBinding_Right,   eKeyBinding_Jump,
        eKeyBinding_Sneak,       eKeyBinding_Sprint,     eKeyBinding_Attack, eKeyBinding_Use,     -1,
        eKeyBinding_Perspective, eKeyBinding_PlayerList, eKeyBinding_Chat,   eKeyBinding_Command, eKeyBinding_Inventory};

void Options::Reset()
{
    gamma                    = 50.0f;
    renderDistance           = 12.0f;
    viewBobbing              = true;
    announceWorldSaves       = true;
    discordRichPresence      = true;
    language                 = 0;
    autosaveIntervalTicks    = 6000;
    mouseSensitivity         = 100.0f;
    controllerSensitivity    = 100.0f;
    menuCursorSensitivity    = 100.0f;
    difficulty               = 2.0f;
    musicVolume              = 100.0f;
    soundVolume              = 100.0f;
    weatherVolume            = 100.0f;
    blockVolume              = 100.0f;
    ambientVolume            = 100.0f;
    playerVolume             = 100.0f;
    uiVolume                 = 100.0f;
    soundRaytracing          = true;
    soundReverb              = 100.0f;
    soundEcho                = 100.0f;
    soundMuffling            = 100.0f;
    vsync                    = true;
    windowMode               = eWindowMode_Windowed;
    renderBackend            = RenderBackend::kBackends[0].key;
    fpsLimit                 = 0;
    renderClouds             = true;
    cloudHeight              = 160;
    entityShadows            = true;
    smartLeaves              = true;
    flatItems                = false;
    occlusionCulling         = true;
    fogCulling               = true;
    frustumCulling           = true;
    gpuChunkCulling          = true;
    smoothLighting           = true;
    ambientOcclusion         = true;
    nimbusEnabled            = false;
    nimbusLighting           = true;
    nimbusClouds             = true;
    nimbusFog                = true;
    nimbusQuality            = 1;
    dynamicLights            = eDynamicLightSetting_Fancy;
    animatedTextures         = true;
    fogDistance              = 100.0f;
    fov                      = 70.0f;
    simulationDistance       = 10;
    entityRenderDistance     = 25;
    particles                = eParticleSetting_All;
    particleLimit            = 4000;
    mipmapLevels             = 3;
    anisotropicFiltering     = 0;
    chunkUploadsPerFrame     = 16;
    chunkWorkerThreads       = 0;
    chunkIntakePerTick       = 8;
    chunkSendsPerTick        = 16;
    chunkSendsInFlight       = 16;
    chunkQueuePerThread      = 4;
    chunkUnloadMargin        = 2;
    lodEnabled               = false;
    lodDistance              = 256;
    lodQuality               = 2;
    lodCpuPreset             = 1;
    lodGenerateTerrain       = true;
    lodTilesInFlight         = 32;
    lodFadeTime              = 300;
    lodTextures              = true;
    lodMipmapLevels          = LodRenderer::s_maxMipmapLevels;
    lodDebug                 = false;
    synchronizedBlockUpdates = true;
    instantBlockUpdates      = false;
    fpsDisplay               = eFpsDisplay_Off;
    onlineGame               = true;
    allowFriendsOfFriends    = false;
    maxPlayers               = 8;
    displayHud               = true;
    displayHand              = true;
    hudSize                  = 3.0f;
    javaTooltips             = false;
    advancedTooltips         = false;
    chatShown                = true;
    chatTextOpacity          = 100.0f;
    chatBackgroundOpacity    = 50.0f;
    chatTextSize             = 100.0f;
    chatLineSpacing          = 0.0f;
    chatHeight               = 50.0f;
    chatWidth                = 100.0f;
    invertLook               = false;
    keyboardToggleSprint     = false;
    keyboardToggleSneak      = false;
    controllerToggleSprint   = false;
    controllerToggleSneak    = true;
    for (size_t i = 0; i < g_keyBindingCount; i++)
    {
        keyBindings[i] = InputManager::GetDefaultBinding((eKEY_BINDING) i);
    }

    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        controllerBindings[i] = s_defaultControllerBindings[i];
    }
}

void Options::ApplyValue(const std::string &key, const std::string &value)
{
    if (key == "gamma")
    {
        gamma = (float) atof(value.c_str());
    }
    else if (key == "render_distance")
    {
        renderDistance = (float) atof(value.c_str());
    }
    else if (key == "view_bobbing")
    {
        viewBobbing = (value == "true");
    }
    else if (key == "announce_world_saves")
    {
        announceWorldSaves = (value == "true");
    }
    else if (key == "language")
    {
        language = (int8_t) atoi(value.c_str());
    }
    else if (key == "autosave_interval_ticks")
    {
        autosaveIntervalTicks = atoi(value.c_str());
    }
    else if (key == "mouse_sensitivity")
    {
        mouseSensitivity = (float) atof(value.c_str());
    }
    else if (key == "controller_sensitivity")
    {
        controllerSensitivity = (float) atof(value.c_str());
    }
    else if (key == "menu_cursor_sensitivity")
    {
        menuCursorSensitivity = (float) atof(value.c_str());
    }
    else if (key == "difficulty")
    {
        difficulty = (float) atof(value.c_str());
    }
    else if (key == "music_volume")
    {
        musicVolume = (float) atof(value.c_str());
    }
    else if (key == "sound_volume")
    {
        soundVolume = (float) atof(value.c_str());
    }
    else if (key == "weather_volume")
    {
        weatherVolume = (float) atof(value.c_str());
    }
    else if (key == "block_volume")
    {
        blockVolume = (float) atof(value.c_str());
    }
    else if (key == "ambient_volume")
    {
        ambientVolume = (float) atof(value.c_str());
    }
    else if (key == "player_volume")
    {
        playerVolume = (float) atof(value.c_str());
    }
    else if (key == "ui_volume")
    {
        uiVolume = (float) atof(value.c_str());
    }
    else if (key == "sound_raytracing")
    {
        soundRaytracing = (value == "true");
    }
    else if (key == "sound_reverb")
    {
        soundReverb = (float) atof(value.c_str());
    }
    else if (key == "sound_echo")
    {
        soundEcho = (float) atof(value.c_str());
    }
    else if (key == "sound_muffling")
    {
        soundMuffling = (float) atof(value.c_str());
    }
    else if (key == "vsync")
    {
        vsync = (value == "true");
    }
    else if (key == "window_mode")
    {
        windowMode = std::clamp(atoi(value.c_str()), (int) eWindowMode_Windowed, (int) eWindowMode_Fullscreen);
    }
    else if (key == "render_backend")
    {
        renderBackend = value;
    }
    else if (key == "fps_limit")
    {
        fpsLimit = std::max(0, atoi(value.c_str()));
    }
    else if (key == "render_clouds")
    {
        renderClouds = (value == "true");
    }
    else if (key == "entity_shadows")
    {
        entityShadows = (value == "true");
    }
    else if (key == "smart_leaves")
    {
        smartLeaves = (value == "true");
    }
    else if (key == "flat_items")
    {
        flatItems = (value == "true");
    }
    else if (key == "occlusion_culling")
    {
        occlusionCulling = (value == "true");
    }
    else if (key == "fog_culling")
    {
        fogCulling = (value == "true");
    }
    else if (key == "discord_rich_presence")
    {
        discordRichPresence = (value == "true");
    }
    else if (key == "frustum_culling")
    {
        frustumCulling = (value == "true");
    }
    else if (key == "gpu_chunk_culling")
    {
        gpuChunkCulling = (value == "true");
    }
    else if (key == "smooth_lighting")
    {
        smoothLighting = (value == "true");
    }
    else if (key == "ambient_occlusion")
    {
        ambientOcclusion = (value == "true");
    }
    else if (key == "nimbus_enabled")
    {
        nimbusEnabled = (value == "true");
    }
    else if (key == "nimbus_lighting")
    {
        nimbusLighting = (value == "true");
    }
    else if (key == "nimbus_clouds")
    {
        nimbusClouds = (value == "true");
    }
    else if (key == "nimbus_fog")
    {
        nimbusFog = (value == "true");
    }
    else if (key == "nimbus_quality")
    {
        nimbusQuality = std::clamp(atoi(value.c_str()), 0, 2);
    }
    else if (key == "dynamic_lights")
    {
        dynamicLights = atoi(value.c_str());
    }
    else if (key == "animated_textures")
    {
        animatedTextures = (value == "true");
    }
    else if (key == "instant_block_updates")
    {
        instantBlockUpdates = (value == "true");
    }
    else if (key == "synchronized_block_updates")
    {
        synchronizedBlockUpdates = (value == "true");
    }
    else if (key == "simulation_distance")
    {
        simulationDistance = atoi(value.c_str());
    }
    else if (key == "entity_render_distance")
    {
        entityRenderDistance = atoi(value.c_str());
    }
    else if (key == "particles")
    {
        particles = atoi(value.c_str());
    }
    else if (key == "particle_limit")
    {
        particleLimit = atoi(value.c_str());
    }
    else if (key == "mipmap_levels")
    {
        mipmapLevels = atoi(value.c_str());
    }
    else if (key == "anisotropic_filtering")
    {
        anisotropicFiltering = atoi(value.c_str());
    }
    else if (key == "chunk_uploads_per_frame")
    {
        chunkUploadsPerFrame = atoi(value.c_str());
    }
    else if (key == "chunk_worker_threads")
    {
        chunkWorkerThreads = atoi(value.c_str());
    }
    else if (key == "chunk_unload_margin")
    {
        chunkUnloadMargin = atoi(value.c_str());
    }
    else if (key == "chunk_intake_per_tick")
    {
        chunkIntakePerTick = atoi(value.c_str());
    }
    else if (key == "chunk_sends_per_tick")
    {
        chunkSendsPerTick = atoi(value.c_str());
    }
    else if (key == "chunk_sends_in_flight")
    {
        chunkSendsInFlight = atoi(value.c_str());
    }
    else if (key == "chunk_queue_per_thread")
    {
        chunkQueuePerThread = atoi(value.c_str());
    }
    else if (key == "lod_enabled")
    {
        lodEnabled = (value == "true");
    }
    else if (key == "lod_distance")
    {
        lodDistance = atoi(value.c_str());
    }
    else if (key == "lod_quality")
    {
        lodQuality = atoi(value.c_str());
    }
    else if (key == "lod_cpu_preset")
    {
        lodCpuPreset = atoi(value.c_str());
    }
    else if (key == "lod_generate_terrain")
    {
        lodGenerateTerrain = (value == "true");
    }
    else if (key == "lod_tiles_in_flight")
    {
        lodTilesInFlight = atoi(value.c_str());
    }
    else if (key == "lod_fade_time")
    {
        lodFadeTime = atoi(value.c_str());
    }
    else if (key == "lod_textures")
    {
        lodTextures = (value == "true");
    }
    else if (key == "lod_mipmap_levels")
    {
        lodMipmapLevels = atoi(value.c_str());
    }
    else if (key == "lod_debug")
    {
        lodDebug = (value == "true");
    }
    else if (key == "fps_display")
    {
        fpsDisplay = atoi(value.c_str());
    }
    else if (key == "cloud_height")
    {
        cloudHeight = atoi(value.c_str());
    }
    else if (key == "fog_distance")
    {
        fogDistance = (float) atof(value.c_str());
    }
    else if (key == "fov")
    {
        fov = (float) atof(value.c_str());
    }
    else if (key == "online_game")
    {
        onlineGame = (value == "true");
    }
    else if (key == "allow_friends_of_friends")
    {
        allowFriendsOfFriends = (value == "true");
    }
    else if (key == "max_players")
    {
        maxPlayers = atoi(value.c_str());
    }
    else if (key == "display_hud")
    {
        displayHud = (value == "true");
    }
    else if (key == "display_hand")
    {
        displayHand = (value == "true");
    }
    else if (key == "hud_size")
    {
        hudSize = std::min((float) atof(value.c_str()), 3.0f);
        if (hudSize >= 2.0f)
        {
            hudSize = 3.0f;
        }
    }
    else if (key == "hud_size_v2")
    {
        hudSize = std::clamp((float) atof(value.c_str()), 1.0f, 3.0f);
    }
    else if (key == "java_tooltips")
    {
        javaTooltips = (value == "true");
    }
    else if (key == "advanced_tooltips")
    {
        advancedTooltips = (value == "true");
    }
    else if (key == "chat_shown")
    {
        chatShown = (value == "true");
    }
    else if (key == "chat_text_opacity")
    {
        chatTextOpacity = (float) atof(value.c_str());
    }
    else if (key == "chat_background_opacity")
    {
        chatBackgroundOpacity = (float) atof(value.c_str());
    }
    else if (key == "chat_text_size")
    {
        chatTextSize = (float) atof(value.c_str());
    }
    else if (key == "chat_line_spacing")
    {
        chatLineSpacing = (float) atof(value.c_str());
    }
    else if (key == "chat_height")
    {
        chatHeight = (float) atof(value.c_str());
    }
    else if (key == "chat_width")
    {
        chatWidth = (float) atof(value.c_str());
    }
    else if (key == "invert_look")
    {
        invertLook = (value == "true");
    }
    else if (key == "keyboard_toggle_sprint")
    {
        keyboardToggleSprint = (value == "true");
    }
    else if (key == "keyboard_toggle_sneak")
    {
        keyboardToggleSneak = (value == "true");
    }
    else if (key == "controller_toggle_sprint")
    {
        controllerToggleSprint = (value == "true");
    }
    else if (key == "controller_toggle_sneak")
    {
        controllerToggleSneak = (value == "true");
    }
    else if (key.rfind("key_", 0) == 0)
    {
        for (size_t i = 0; i < g_keyBindingCount; i++)
        {
            if (key.compare(4, std::string::npos, s_keyBindingNames[i]) == 0)
            {
                keyBindings[i] = atoi(value.c_str());
            }
        }
    }
    else if (key.rfind("controller_", 0) == 0)
    {
        for (size_t i = 0; i < g_controllerBindingCount; i++)
        {
            int button = atoi(value.c_str());
            if (key.compare(11, std::string::npos, s_controllerBindingNames[i]) == 0 && button >= 0 && button < (int) g_controllerButtonCount)
            {
                controllerBindings[i] = button;
            }
        }
    }
    else if (key.rfind("bind_", 0) == 0)
    {
        int index = atoi(key.c_str() + 5);
        if (index >= 0 && index < s_legacyBindingCount && s_legacyBindings[index] >= 0)
        {
            keyBindings[s_legacyBindings[index]] = atoi(value.c_str());
        }
    }
}

void Options::Load()
{
    File optionsFile = FileSystem::GetUserDirectory().GetChild("options.txt");
    if (!optionsFile.Exists() || !optionsFile.IsRegularFile())
    {
        return;
    }

    std::string text = optionsFile.ReadAllText();
    if (text.empty())
    {
        return;
    }

    std::vector<std::string> lines = StringUtils::Split(text, '\n');
    for (size_t i = 0; i < lines.size(); i++)
    {
        std::string line = StringUtils::Trim(lines[i]);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        std::string key   = StringUtils::Trim(line.substr(0, separator));
        std::string value = StringUtils::Trim(line.substr(separator + 1));
        if (key.empty())
        {
            continue;
        }

        ApplyValue(key, value);
    }

    Logger::LogInfo("Options loaded path=%s", optionsFile.GetPath().c_str());
}

void Options::Save() const
{
    std::string content;
    content += "gamma=" + StringUtils::FloatToString(gamma) + "\n";
    content += "render_distance=" + StringUtils::FloatToString(renderDistance) + "\n";
    content += "view_bobbing=" + StringUtils::BoolToString(viewBobbing) + "\n";
    content += "announce_world_saves=" + StringUtils::BoolToString(announceWorldSaves) + "\n";
    content += "language=" + std::to_string((int) language) + "\n";
    content += "autosave_interval_ticks=" + std::to_string(autosaveIntervalTicks) + "\n";
    content += "mouse_sensitivity=" + StringUtils::FloatToString(mouseSensitivity) + "\n";
    content += "controller_sensitivity=" + StringUtils::FloatToString(controllerSensitivity) + "\n";
    content += "menu_cursor_sensitivity=" + StringUtils::FloatToString(menuCursorSensitivity) + "\n";
    content += "difficulty=" + StringUtils::FloatToString(difficulty) + "\n";
    content += "music_volume=" + StringUtils::FloatToString(musicVolume) + "\n";
    content += "sound_volume=" + StringUtils::FloatToString(soundVolume) + "\n";
    content += "weather_volume=" + StringUtils::FloatToString(weatherVolume) + "\n";
    content += "block_volume=" + StringUtils::FloatToString(blockVolume) + "\n";
    content += "ambient_volume=" + StringUtils::FloatToString(ambientVolume) + "\n";
    content += "player_volume=" + StringUtils::FloatToString(playerVolume) + "\n";
    content += "ui_volume=" + StringUtils::FloatToString(uiVolume) + "\n";
    content += "sound_raytracing=" + StringUtils::BoolToString(soundRaytracing) + "\n";
    content += "sound_reverb=" + StringUtils::FloatToString(soundReverb) + "\n";
    content += "sound_echo=" + StringUtils::FloatToString(soundEcho) + "\n";
    content += "sound_muffling=" + StringUtils::FloatToString(soundMuffling) + "\n";
    content += "vsync=" + StringUtils::BoolToString(vsync) + "\n";
    content += "window_mode=" + std::to_string(windowMode) + "\n";
    content += "render_backend=" + renderBackend + "\n";
    content += "fps_limit=" + std::to_string(fpsLimit) + "\n";
    content += "render_clouds=" + StringUtils::BoolToString(renderClouds) + "\n";
    content += "cloud_height=" + std::to_string(cloudHeight) + "\n";
    content += "entity_shadows=" + StringUtils::BoolToString(entityShadows) + "\n";
    content += "smart_leaves=" + StringUtils::BoolToString(smartLeaves) + "\n";
    content += "flat_items=" + StringUtils::BoolToString(flatItems) + "\n";
    content += "occlusion_culling=" + StringUtils::BoolToString(occlusionCulling) + "\n";
    content += "fog_culling=" + StringUtils::BoolToString(fogCulling) + "\n";
    content += "frustum_culling=" + StringUtils::BoolToString(frustumCulling) + "\n";
    content += "gpu_chunk_culling=" + StringUtils::BoolToString(gpuChunkCulling) + "\n";
    content += "discord_rich_presence=" + StringUtils::BoolToString(discordRichPresence) + "\n";
    content += "smooth_lighting=" + StringUtils::BoolToString(smoothLighting) + "\n";
    content += "ambient_occlusion=" + StringUtils::BoolToString(ambientOcclusion) + "\n";
    content += "nimbus_enabled=" + StringUtils::BoolToString(nimbusEnabled) + "\n";
    content += "nimbus_lighting=" + StringUtils::BoolToString(nimbusLighting) + "\n";
    content += "nimbus_clouds=" + StringUtils::BoolToString(nimbusClouds) + "\n";
    content += "nimbus_fog=" + StringUtils::BoolToString(nimbusFog) + "\n";
    content += "nimbus_quality=" + std::to_string(nimbusQuality) + "\n";
    content += "dynamic_lights=" + std::to_string(dynamicLights) + "\n";
    content += "animated_textures=" + StringUtils::BoolToString(animatedTextures) + "\n";
    content += "synchronized_block_updates=" + StringUtils::BoolToString(synchronizedBlockUpdates) + "\n";
    content += "instant_block_updates=" + StringUtils::BoolToString(instantBlockUpdates) + "\n";
    content += "simulation_distance=" + std::to_string(simulationDistance) + "\n";
    content += "entity_render_distance=" + std::to_string(entityRenderDistance) + "\n";
    content += "particles=" + std::to_string(particles) + "\n";
    content += "particle_limit=" + std::to_string(particleLimit) + "\n";
    content += "mipmap_levels=" + std::to_string(mipmapLevels) + "\n";
    content += "anisotropic_filtering=" + std::to_string(anisotropicFiltering) + "\n";
    content += "chunk_uploads_per_frame=" + std::to_string(chunkUploadsPerFrame) + "\n";
    content += "chunk_worker_threads=" + std::to_string(chunkWorkerThreads) + "\n";
    content += "chunk_unload_margin=" + std::to_string(chunkUnloadMargin) + "\n";
    content += "chunk_intake_per_tick=" + std::to_string(chunkIntakePerTick) + "\n";
    content += "chunk_sends_per_tick=" + std::to_string(chunkSendsPerTick) + "\n";
    content += "chunk_sends_in_flight=" + std::to_string(chunkSendsInFlight) + "\n";
    content += "chunk_queue_per_thread=" + std::to_string(chunkQueuePerThread) + "\n";
    content += "lod_enabled=" + StringUtils::BoolToString(lodEnabled) + "\n";
    content += "lod_distance=" + std::to_string(lodDistance) + "\n";
    content += "lod_quality=" + std::to_string(lodQuality) + "\n";
    content += "lod_cpu_preset=" + std::to_string(lodCpuPreset) + "\n";
    content += "lod_generate_terrain=" + StringUtils::BoolToString(lodGenerateTerrain) + "\n";
    content += "lod_tiles_in_flight=" + std::to_string(lodTilesInFlight) + "\n";
    content += "lod_fade_time=" + std::to_string(lodFadeTime) + "\n";
    content += "lod_textures=" + StringUtils::BoolToString(lodTextures) + "\n";
    content += "lod_mipmap_levels=" + std::to_string(lodMipmapLevels) + "\n";
    content += "lod_debug=" + StringUtils::BoolToString(lodDebug) + "\n";
    content += "fps_display=" + std::to_string(fpsDisplay) + "\n";
    content += "fog_distance=" + StringUtils::FloatToString(fogDistance) + "\n";
    content += "fov=" + StringUtils::FloatToString(fov) + "\n";
    content += "online_game=" + StringUtils::BoolToString(onlineGame) + "\n";
    content += "allow_friends_of_friends=" + StringUtils::BoolToString(allowFriendsOfFriends) + "\n";
    content += "max_players=" + std::to_string(maxPlayers) + "\n";
    content += "display_hud=" + StringUtils::BoolToString(displayHud) + "\n";
    content += "display_hand=" + StringUtils::BoolToString(displayHand) + "\n";
    content += "hud_size_v2=" + StringUtils::FloatToString(hudSize) + "\n";
    content += "java_tooltips=" + StringUtils::BoolToString(javaTooltips) + "\n";
    content += "advanced_tooltips=" + StringUtils::BoolToString(advancedTooltips) + "\n";
    content += "chat_shown=" + StringUtils::BoolToString(chatShown) + "\n";
    content += "chat_text_opacity=" + StringUtils::FloatToString(chatTextOpacity) + "\n";
    content += "chat_background_opacity=" + StringUtils::FloatToString(chatBackgroundOpacity) + "\n";
    content += "chat_text_size=" + StringUtils::FloatToString(chatTextSize) + "\n";
    content += "chat_line_spacing=" + StringUtils::FloatToString(chatLineSpacing) + "\n";
    content += "chat_height=" + StringUtils::FloatToString(chatHeight) + "\n";
    content += "chat_width=" + StringUtils::FloatToString(chatWidth) + "\n";
    content += "invert_look=" + StringUtils::BoolToString(invertLook) + "\n";
    content += "keyboard_toggle_sprint=" + StringUtils::BoolToString(keyboardToggleSprint) + "\n";
    content += "keyboard_toggle_sneak=" + StringUtils::BoolToString(keyboardToggleSneak) + "\n";
    content += "controller_toggle_sprint=" + StringUtils::BoolToString(controllerToggleSprint) + "\n";
    content += "controller_toggle_sneak=" + StringUtils::BoolToString(controllerToggleSneak) + "\n";
    for (size_t i = 0; i < g_keyBindingCount; i++)
    {
        content += std::string("key_") + s_keyBindingNames[i] + "=" + std::to_string(keyBindings[i]) + "\n";
    }

    for (size_t i = 0; i < g_controllerBindingCount; i++)
    {
        content += std::string("controller_") + s_controllerBindingNames[i] + "=" + std::to_string(controllerBindings[i]) + "\n";
    }

    File optionsFile          = FileSystem::GetUserDirectory().GetChild("options.txt");
    std::vector<uint8_t> data = std::vector<uint8_t>(content.begin(), content.end());
    optionsFile.WriteAllBytes(data);
}

// This is indexed by eSOUND_CATEGORY
float Options::GetCategoryVolume(uint8_t category) const
{
    const float volumes[] = {musicVolume, weatherVolume, blockVolume, ambientVolume, playerVolume, uiVolume};
    if (category >= sizeof(volumes) / sizeof(volumes[0]))
    {
        return 100.0f;
    }

    return volumes[category];
}

float Options::GetHudScale() const
{
    return 1.5f + hudSize * 0.5f;
}

#endif
