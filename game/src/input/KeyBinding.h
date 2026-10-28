#pragma once

#include <cstddef>

enum eKEY_BINDING : size_t
{
    eKeyBinding_Forward               = 0,
    eKeyBinding_Back                  = 1,
    eKeyBinding_Left                  = 2,
    eKeyBinding_Right                 = 3,
    eKeyBinding_Jump                  = 4,
    eKeyBinding_Sneak                 = 5,
    eKeyBinding_Sprint                = 6,
    eKeyBinding_Attack                = 7,
    eKeyBinding_Use                   = 8,
    eKeyBinding_Perspective           = 9,
    eKeyBinding_PlayerList            = 10,
    eKeyBinding_Chat                  = 11,
    eKeyBinding_Command               = 12,
    eKeyBinding_Inventory             = 13,
    eKeyBinding_PickBlock             = 14,
    eKeyBinding_Drop                  = 15,
    eKeyBinding_ViewRecipe            = 16,
    eKeyBinding_DebugMenu             = 17,
    eKeyBinding_DebugHitboxes         = 18,
    eKeyBinding_DebugChunkBorders     = 19,
    eKeyBinding_DebugAdvancedTooltips = 20,
    eKeyBinding_Screenshot            = 21,
    eKeyBinding_Panorama              = 22,
    eKeyBinding_HideHud               = 23,
    eKeyBinding_DebugReloadChunks     = 24,
    eKeyBinding_Count                 = 25,
};

const size_t g_keyBindingCount = (size_t) eKeyBinding_Count;

static const int s_mouseButtonInputBase = 2000;

static const char *const s_keyBindingNames[g_keyBindingCount] = {"forward",
                                                                 "back",
                                                                 "left",
                                                                 "right",
                                                                 "jump",
                                                                 "sneak",
                                                                 "sprint",
                                                                 "attack",
                                                                 "use",
                                                                 "perspective",
                                                                 "player_list",
                                                                 "chat",
                                                                 "command",
                                                                 "inventory",
                                                                 "pick_block",
                                                                 "drop",
                                                                 "view_recipe",
                                                                 "debug_menu",
                                                                 "debug_hitboxes",
                                                                 "debug_chunk_borders",
                                                                 "debug_advanced_tooltips",
                                                                 "screenshot",
                                                                 "panorama",
                                                                 "hide_hud",
                                                                 "debug_reload_chunks"};
