#pragma once

#include <string>

enum eGAME_TYPE
{
    eGameType_Survival  = 0,
    eGameType_Creative  = 1,
    eGameType_Adventure = 2,
    eGameType_Spectator = 3,
    eGameType_Count     = 4,
};

class GameType
{
public:
    static bool CanFly(eGAME_TYPE gameType);
    static bool MayBuild(eGAME_TYPE gameType);
    static bool IsSpectator(eGAME_TYPE gameType);
    static bool CanHurtPlayer(eGAME_TYPE gameType);
    static bool HasInfiniteItems(eGAME_TYPE gameType);
    static bool IsValid(int gameType);
    static bool Parse(const std::string &text, eGAME_TYPE *gameType);
    static const char *GetName(eGAME_TYPE gameType);
};
