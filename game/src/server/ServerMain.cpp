#ifdef EVERSTEAD_DEDICATED_SERVER

#include <csignal>
#include <cstdlib>
#include <cstring>

#include "../core/CrashHandler.h"
#include "../core/Logger.h"
#include "../io/File.h"
#include "../mod/ModLoader.h"
#include "../platform/PlatformProcess.h"
#include "DedicatedServer.h"
#include "ServerAssets.h"

static void HandleSignal(int signal)
{
    DedicatedServer::RequestStop();
}

int main(int argc, char *argv[])
{
    PlatformProcess::ConfigureAllocator();
    std::string configPath = "server.json";
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
        {
            configPath = argv[++i];
        }
    }

    File configFile(configPath);
    Logger::Init(configFile.GetParent().GetChild("logs"));
    if (!ServerAssets::Update())
    {
        Logger::Shutdown();
        return 1;
    }

    CrashHandler::Install(true);
    ModLoader::GetInstance()->Load(configFile.GetParent().GetChild("mods"));

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    int result = DedicatedServer(configFile).Run();
    Logger::Shutdown();
    // The console thread stays blocked in getline holding the stdin lock, so a normal exit hangs on stdio cleanup
    std::_Exit(result);
}

#endif
