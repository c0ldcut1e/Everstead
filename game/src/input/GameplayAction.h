#pragma once

#include <cstddef>

enum eGAMEPLAY_ACTION : size_t
{
    eGameplayAction_Jump                   = 0,
    eGameplayAction_BreakBlock             = 1,
    eGameplayAction_PlaceBlock             = 2,
    eGameplayAction_Back                   = 3,
    eGameplayAction_PlayerList             = 4,
    eGameplayAction_ToggleDebugOverlay     = 5,
    eGameplayAction_ToggleHud              = 6,
    eGameplayAction_TogglePerspective      = 7,
    eGameplayAction_Sprint                 = 8,
    eGameplayAction_Sneak                  = 9,
    eGameplayAction_ToggleHitboxes         = 10,
    eGameplayAction_ToggleChunkBorders     = 11,
    eGameplayAction_Chat                   = 12,
    eGameplayAction_Command                = 13,
    eGameplayAction_ToggleProfiler         = 14,
    eGameplayAction_ToggleInventory        = 15,
    eGameplayAction_Screenshot             = 16,
    eGameplayAction_Panorama               = 17,
    eGameplayAction_PickBlock              = 18,
    eGameplayAction_DropItem               = 19,
    eGameplayAction_DropStack              = 20,
    eGameplayAction_ToggleAdvancedTooltips = 21,
    eGameplayAction_ReloadChunks           = 22,
    eGameplayAction_Count                  = 23,
};

const size_t g_gameplayActionCount = (size_t) eGameplayAction_Count;
