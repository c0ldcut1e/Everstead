#ifndef EVERSTEAD_DEDICATED_SERVER

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <SDL2/SDL_main.h>

#include "core/CrashHandler.h"
#include "core/EversteadApp.h"
#include "core/Logger.h"
#include "io/FileSystem.h"
#include "mod/ModLoader.h"
#include "network/ApiClient.h"
#include "platform/PlatformProcess.h"

int main(int argc, char *argv[])
{
    PlatformProcess::ConfigureAllocator();

    Logger::Init(FileSystem::GetUserDirectory().GetChild("logs"));
    Logger::LogInfo("Logger initialized");
    CrashHandler::Install(false);

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "--server=", 9) == 0)
        {
            ApiClient::SetServerOverride(argv[i] + 9);
        }
        else if (strcmp(argv[i], "--server") == 0 && i + 1 < argc)
        {
            ApiClient::SetServerOverride(argv[++i]);
        }
    }

    ModLoader::GetInstance()->Load();

    try
    {
        EversteadApp::GetInstance()->Start();
    }
    catch (const std::exception &exception)
    {
        Logger::LogError("Caught an unexpected exception: %s", exception.what());
        return -1;
    }

    Logger::Shutdown();

    return 0;
}

#endif
