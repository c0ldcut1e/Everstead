#pragma once

#include <cstddef>

#include "ControllerGlyphs.h"

enum eCONTROLLER_BINDING : size_t
{
    eControllerBinding_Jump              = 0,
    eControllerBinding_Inventory         = 1,
    eControllerBinding_Attack            = 2,
    eControllerBinding_Use               = 3,
    eControllerBinding_HotbarNext        = 4,
    eControllerBinding_HotbarPrevious    = 5,
    eControllerBinding_Perspective       = 6,
    eControllerBinding_Sneak             = 7,
    eControllerBinding_Sprint            = 8,
    eControllerBinding_Chat              = 9,
    eControllerBinding_PlayerList        = 10,
    eControllerBinding_Pause             = 11,
    eControllerBinding_PickBlock         = 12,
    eControllerBinding_Drop              = 13,
    eControllerBinding_DebugMenu         = 14,
    eControllerBinding_DebugProfiler     = 15,
    eControllerBinding_DebugHitboxes     = 16,
    eControllerBinding_DebugChunkBorders = 17,
    eControllerBinding_Screenshot        = 18,
    eControllerBinding_Panorama          = 19,
    eControllerBinding_HideHud           = 20,
    eControllerBinding_Count             = 21,
};

const size_t g_controllerBindingCount = (size_t) eControllerBinding_Count;

static const char *const s_controllerBindingNames[g_controllerBindingCount] = {"jump",           "inventory",
                                                                               "attack",         "use",
                                                                               "hotbar_next",    "hotbar_previous",
                                                                               "perspective",    "sneak",
                                                                               "sprint",         "chat",
                                                                               "player_list",    "pause",
                                                                               "pick_block",     "drop",
                                                                               "debug_menu",     "debug_profiler",
                                                                               "debug_hitboxes", "debug_chunk_borders",
                                                                               "screenshot",     "panorama",
                                                                               "hide_hud"};

static const eCONTROLLER_BUTTON s_defaultControllerBindings[g_controllerBindingCount] = {
        eControllerButton_South,         eControllerButton_North,        eControllerButton_RightTrigger, eControllerButton_LeftTrigger,
        eControllerButton_RightShoulder, eControllerButton_LeftShoulder, eControllerButton_LeftStick,    eControllerButton_RightStick,
        eControllerButton_Count,         eControllerButton_DpadUp,       eControllerButton_Back,         eControllerButton_Start,
        eControllerButton_DpadDown,      eControllerButton_East,         eControllerButton_Count,        eControllerButton_Count,
        eControllerButton_Count,         eControllerButton_Count,        eControllerButton_Count,        eControllerButton_Count,
        eControllerButton_Count};
