#pragma once

#include <cstdint>

enum eDISCONNECT_REASON : uint8_t
{
    eDisconnectReason_None             = 0,
    eDisconnectReason_Closed           = 1,
    eDisconnectReason_LoginTooLong     = 2,
    eDisconnectReason_UnexpectedPacket = 3,
    eDisconnectReason_Timeout          = 4,
    eDisconnectReason_MaxPlayers       = 5,
    eDisconnectReason_ServerStopping   = 6,
    eDisconnectReason_OlderVersion     = 7,
    eDisconnectReason_NewerVersion     = 8,
    eDisconnectReason_Banned           = 9,
    eDisconnectReason_SignedOut        = 10,
    eDisconnectReason_SessionExpired   = 11,
    eDisconnectReason_Kicked           = 12,
    eDisconnectReason_NotWhitelisted   = 13,
};
