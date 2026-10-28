#include "GameType.h"

#include "../../utils/StringUtils.h"

bool GameType::CanFly(eGAME_TYPE gameType)
{
    return gameType == eGameType_Creative || gameType == eGameType_Spectator;
}

bool GameType::MayBuild(eGAME_TYPE gameType)
{
    return gameType == eGameType_Survival || gameType == eGameType_Creative;
}

bool GameType::IsSpectator(eGAME_TYPE gameType)
{
    return gameType == eGameType_Spectator;
}

bool GameType::CanHurtPlayer(eGAME_TYPE gameType)
{
    return gameType == eGameType_Survival || gameType == eGameType_Adventure;
}

bool GameType::HasInfiniteItems(eGAME_TYPE gameType)
{
    return gameType == eGameType_Creative;
}

bool GameType::IsValid(int gameType)
{
    return gameType >= 0 && gameType < eGameType_Count;
}

bool GameType::Parse(const std::string &text, eGAME_TYPE *gameType)
{
    std::string lower = StringUtils::ToLower(text);
    if (lower.size() == 1 && lower[0] >= '0' && lower[0] <= '9' && IsValid(lower[0] - '0'))
    {
        *gameType = (eGAME_TYPE) (lower[0] - '0');
        return true;
    }

    for (int i = 0; i < eGameType_Count; i++)
    {
        if (lower == GetName((eGAME_TYPE) i))
        {
            *gameType = (eGAME_TYPE) i;
            return true;
        }
    }

    if (lower == "spectater")
    {
        *gameType = eGameType_Spectator;
        return true;
    }

    return false;
}

const char *GameType::GetName(eGAME_TYPE gameType)
{
    switch (gameType)
    {
        case eGameType_Creative: {
            return "creative";
        }
        case eGameType_Adventure: {
            return "adventure";
        }
        case eGameType_Spectator: {
            return "spectator";
        }
        default: {
            return "survival";
        }
    }
}
